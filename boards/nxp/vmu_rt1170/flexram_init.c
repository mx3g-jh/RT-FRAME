/*
 * FlexRAM bank reallocation: 12 ITCM (384K) + 4 DTCM (128K).
 * Default fuse is 8/8/0 (256K/256K/0). Software override via GPR17/GPR18/GPR16.
 * Banks 0-3 = DTCM (10b), banks 4-15 = ITCM (11b).
 * Must run before .itcm section copy in Reset_Handler, so placed at PRE_KERNEL_1 prio 0.
 */

#include <zephyr/init.h>
#include <fsl_iomuxc.h>

static int flexram_reallocate(void)
{
	/* Banks 0-3: DTCM (2), banks 4-15: ITCM (3)
	 * GPR17 = banks 0-7 (2 bits each): 0b11_11_11_11_10_10_10_10 = 0xFFAAU
	 * GPR18 = banks 8-15 (2 bits each): 0b11_11_11_11_11_11_11_11 = 0xFFFFU
	 */
	IOMUXC_GPR->GPR17 = (IOMUXC_GPR->GPR17 & ~IOMUXC_GPR_GPR17_FLEXRAM_BANK_CFG_LOW_MASK)
			  | 0xFFAAU;
	IOMUXC_GPR->GPR18 = (IOMUXC_GPR->GPR18 & ~IOMUXC_GPR_GPR18_FLEXRAM_BANK_CFG_HIGH_MASK)
			  | 0xFFFFU;
	IOMUXC_GPR->GPR16 |= IOMUXC_GPR_GPR16_FLEXRAM_BANK_CFG_SEL_MASK;

	__DSB();
	__ISB();

	return 0;
}

SYS_INIT(flexram_reallocate, PRE_KERNEL_1, 0);
