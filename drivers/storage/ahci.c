#include "../kernel/kernel.h"
#include "../kernel/io.h"
#include "../kernel/memory.h"
#include <string.h>

// AHCI Registers
#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3
#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_ATA 0x00000101

typedef volatile struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
} hba_port_t;

typedef volatile struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t bccc;
    uint32_t bccd;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsv[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    hba_port_t ports[32];
} hba_mem_t;

static hba_mem_t* hba_base;

void ahci_init(uint64_t base) {
    hba_base = (hba_mem_t*)base;
    kprintf("AHCI Controller initialized at 0x%lx\n", base);
    
    uint32_t pi = hba_base->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            int det = hba_base->ports[i].ssts & 0x0F;
            int ipm = (hba_base->ports[i].ssts >> 8) & 0x0F;
            
            if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE) {
                kprintf("SATA device found on port %d, sig: 0x%x\n", i, hba_base->ports[i].sig);
            }
        }
    }
}
