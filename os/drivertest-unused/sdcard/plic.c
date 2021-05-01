// #include "param.h"
// #include "riscv.h"
#include "mm.h"
#include "sbi.h"
#include "plic.h"
#include "cpu.h"
#include "printk.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

int cpuid()
{
  return gethartid(getcpu());
}

void plicinit(void)
{
  (*(volatile uint32 *)(PLIC_V + DISK_IRQ * sizeof(uint32))) = 1;
  (*(volatile uint32 *)(PLIC_V + UART_IRQ * sizeof(uint32))) = 1;

	#ifdef DEBUG 
	printf("plicinit\n");
	#endif 
}

void
plicinithart(void)
{
  int hart = cpuid();
  #ifdef _QEMU
  // set uart's enable bit for this hart's S-mode. 
  *(uint32*)PLIC_SENABLE(hart)= (1 << UART_IRQ) | (1 << DISK_IRQ);
  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
  #else
  uint32 *hart_m_enable = (uint32*)PLIC_MENABLE(hart);
  *(hart_m_enable) = (*(volatile uint32 *)((uint64)(hart_m_enable) | (1 << DISK_IRQ)));
  uint32 *hart0_m_int_enable_hi = hart_m_enable + 1;
  *(hart0_m_int_enable_hi) = (*(volatile uint32 *)(hart0_m_int_enable_hi)) | (1 << (UART_IRQ % 32));
  #endif
  #ifdef DEBUG
  printf("plicinithart\n");
  #endif
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int hart = cpuid();
  int irq;
  #ifndef _QEMU
  irq = *(uint32*)PLIC_MCLAIM(hart);
  #else
  irq = *(uint32*)PLIC_SCLAIM(hart);
  #endif
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = cpuid();
  #ifndef _QEMU
  *(uint32*)PLIC_MCLAIM(hart) = irq;
  #else
  *(uint32*)PLIC_SCLAIM(hart) = irq;
  #endif
}

