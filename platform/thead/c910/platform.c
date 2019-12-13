/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Yibin Liu <yibin_liu@c-sky.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/sys/clint.h>
#include <sbi_utils/serial/uart8250.h>

#define C910_HART_COUNT   4
#define C910_HART_STACK_SIZE   8192

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

void *c910_plic_base_addr;
void *c910_clint_base_addr;
static int c910_early_init(bool cold_boot)
{
	static long pmpaddr0, pmpaddr1, pmpaddr2, pmpaddr3, pmpaddr4, pmpaddr5, pmpaddr6, pmpaddr7;
	static long pmpcfg0;
	static long mcor, mhcr, mccr2, mhint, mxstatus;

	if (cold_boot) {
		//Load from boot core
		pmpaddr0 = csr_read(CSR_PMPADDR0);
		pmpaddr1 = csr_read(CSR_PMPADDR1);
		pmpaddr2 = csr_read(CSR_PMPADDR2);
		pmpaddr3 = csr_read(CSR_PMPADDR3);
		pmpaddr4 = csr_read(CSR_PMPADDR4);
		pmpaddr5 = csr_read(CSR_PMPADDR5);
		pmpaddr6 = csr_read(CSR_PMPADDR6);
		pmpaddr7 = csr_read(CSR_PMPADDR7);
		pmpcfg0  = csr_read(CSR_PMPCFG0);

		mcor     = csr_read(CSR_MCOR);
		mhcr     = csr_read(CSR_MHCR);
		mccr2    = csr_read(CSR_MCCR2);
		mhint    = csr_read(CSR_MHINT);
		mxstatus = csr_read(CSR_MXSTATUS);
	} else {
		//Store to other core
		csr_write(CSR_PMPADDR0, pmpaddr0);
		csr_write(CSR_PMPADDR1, pmpaddr1);
		csr_write(CSR_PMPADDR2, pmpaddr2);
		csr_write(CSR_PMPADDR3, pmpaddr3);
		csr_write(CSR_PMPADDR4, pmpaddr4);
		csr_write(CSR_PMPADDR5, pmpaddr5);
		csr_write(CSR_PMPADDR6, pmpaddr6);
		csr_write(CSR_PMPADDR7, pmpaddr7);
		csr_write(CSR_PMPCFG0, pmpcfg0);

		csr_write(CSR_MCOR, mcor);
		csr_write(CSR_MHCR, mhcr);
		csr_write(CSR_MCCR2, mccr2);
		csr_write(CSR_MHINT, mhint);
		csr_write(CSR_MXSTATUS, mxstatus);
	}

	c910_plic_base_addr = (void *)csr_read(CSR_PLIC_BASE);
	c910_clint_base_addr = (void *)((long)c910_plic_base_addr + 0x04000000);

	return 0;
}

static int c910_final_init(bool cold_boot)
{
	return 0;
}

static int c910_irqchip_init(bool cold_boot)
{
	//Delegate plic enable into S-mode
	writel(1, c910_plic_base_addr + 0x001ffffc);

	return 0;
}

static int c910_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init((unsigned long)c910_clint_base_addr,
					 C910_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static volatile u64 *c910_time_cmp;
static int c910_timer_init(bool cold_boot)
{
	c910_time_cmp = (u64 *)(c910_clint_base_addr + 0x4000);

	return 0;
}

static void c910_timer_event_start(u64 next_event)
{
	u32 target_hart = sbi_current_hartid();

	if (C910_HART_COUNT <= target_hart)
		return;

	writel_relaxed(next_event, &c910_time_cmp[target_hart]);
	writel_relaxed(next_event >> 32,
				(void *)(&c910_time_cmp[target_hart]) + 0x04);
}

static int c910_system_shutdown(u32 type)
{
	asm volatile ("ebreak");
	return 0;
}

void sbi_boot_other_core(int hartid)
{
	csr_write(CSR_MRVBR, FW_TEXT_START);
	csr_write(CSR_MRMR, csr_read(CSR_MRMR) | (1 << hartid));
}

#define SBI_EXT_0_1_BOOT_OTHER_CORE    0x09000003

static int c910_vendor_ext_provider(long extid, long funcid,
				unsigned long *args, unsigned long *out_value,
				unsigned long *out_trap_cause,
				unsigned long *out_trap_val)
{
	switch (extid) {
	case SBI_EXT_0_1_BOOT_OTHER_CORE:
		sbi_boot_other_core((int)args[0]);
		break;
	default:
		sbi_printf("Unsupported private sbi call: %ld\n", extid);
		asm volatile ("ebreak");
	}
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.early_init          = c910_early_init,
	.final_init          = c910_final_init,

	.irqchip_init        = c910_irqchip_init,

	.ipi_init            = c910_ipi_init,
	.ipi_send            = clint_ipi_send,
	.ipi_clear           = clint_ipi_clear,

	.timer_init          = c910_timer_init,
	.timer_event_start   = c910_timer_event_start,

	.system_shutdown     = c910_system_shutdown,

	.vendor_ext_provider = c910_vendor_ext_provider,
};

const struct sbi_platform platform = {
	.opensbi_version     = OPENSBI_VERSION,
	.platform_version    = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name                = "Thead C910",
	.features            = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count          = C910_HART_COUNT,
	.hart_stack_size     = C910_HART_STACK_SIZE,
	.disabled_hart_mask  = 0,
	.platform_ops_addr   = (unsigned long)&platform_ops
};
