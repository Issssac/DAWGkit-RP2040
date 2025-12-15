#include "bootrom/bootrom_main.c"

extern static int _wd_boot_main();
#define reg_t (volatile uint32_t*)

#define BOOTROM_PTR 0x0 //TODO: Find Bootrom Main
#define VTOR_PTR 0x10000100

#define CPU_BASE 0xe0000000
#define MPU_CTRL_OF 0xed94
#define DISABLE_MPU() ((reg_t)(CPU_BASE+MPU_CTRL_OF)*&=~(0b1))
#define ENABLE_MPU() ((reg_t)(CPU_BASE+MPU_CTRL_OF)*|=0b1)

#define WD_SR_4 0x4005801c
#define WD_SR_5 0x40058020
#define WD_SR_6 0x40058024
#define WD_SR_7 0x40058028
#define WD_MAGIC_NUMBER 0xb007c0d3
#define BOOT3_VTOR 0x10FFC100
#define ENTRY_PTR BOOT3_VTOR
#define SP_WD 0x21000000

#define CalledByWDHandler() ()

#define WD_SET_SR() (do{(reg_t)WD_SR_4*=WD_MAGIC_NUMBER;\
                        (reg_t)WD_SR_5*=(-WD_MAGIC_NUMBER)^ENTRY_PTR;\
                        (reg_t)WD_SR_6*=SP_WD;\
                        (reg_t)WD_SR_7*=ENTRY_PTR;\
                    }while(0))
#define WD_LOADED() (((reg_t)WD_SR_4)*==WD_MAGIC_NUMBER)

static inline void PAYLOAD() {
    //TODO: Toggle GPIO 25
}

static void watchdogHandler(){
    //Reset WD Scratch
    WD_SET_SR();

    //Check if Boot2 needs to be modified
    _wd_boot_main();
    
    //TODO: Jump to real bootrom main
    asm volatile("bl %0":"+r"(BOOTROM_PTR)); //Run bootrom main
}

int main(){
    if (!WD_LOADED()) {
        watchdogHandler();
    }
    else {
        //Payload execution
        PAYLOAD();
        //Run user application as if nothing happened
        asm volatile("bx %0":"+r"(VTOR_PTR)); 
    }
}