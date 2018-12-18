#include "fpga_bridges.h"
#include "klog.h"

/*
https://www.altera.com/documentation/sfo1410070178831.html

brgmodrst
The BRGMODRST register is used by software to trigger module resets (individual module reset signals). 
Software explicitly asserts and de-asserts module reset signals by writing bits in the appropriate *MODRST register. 
It is up to software to ensure module reset signals are asserted for the appropriate length of time and are de-asserted in the correct order. 
It is also up to software to not assert a module reset signal that would prevent software from de-asserting the module reset signal. 
For example, software should not assert the module reset to the CPU executing the software.

Software writes a bit to 1 to assert the module reset signal and to 0 to de-assert the module reset signal.

All fields are reset by a cold reset.
All fields are also reset by a warm reset if not masked by the corresponding BRGWARMMASK field.
The reset value of all fields is 1. This holds the corresponding module in reset until software is ready to release the module from reset by writing 0 to its field.

Module Instance		Base Address	Register Address
i_rst_mgr_rstmgr	0xFFD05000		0xFFD0502C
									Offset: 0x2C

Access: RW

Important: The value of a reserved bit must be maintained in software. 
When you modify registers containing reserved bit fields, you must use a read-modify-write operation to preserve state and prevent indeterminate system behavior.

brgmodrst Fields
Bit		Name		Description										Access	Reset
31-6	Reserved
6		ddrsch		Resets logic in the DDR Scheduler in the NOC.	RW		0x1
5		f2ssdram2	Resets F2S_SDRAM2 Bridge						RW		0x1
4		f2ssdram1	Resets F2S_SDRAM1 Bridge						RW		0x1
3		f2ssdram0	Resets F2S_SDRAM0 Bridge						RW		0x1
2		fpga2hps	Resets FPGA2HPS Bridge							RW		0x1
1		lwhps2fpga	Resets LWHPS2FPGA Bridge						RW		0x1
0		hps2fpga	Resets HPS2FPGA Bridge							RW		0x1
*/
#define BRGMODRST_ADDR 0xFFD0502C

//everything except the last 6 bits: f2ssdram2, f2ssdram1, f2ssdram0, fpga2hps, lwhps2fpga, hps2fpga
#define BRGMODRST_MASK 0xFFFFFFC0; 


bool enable_fpga_bridges(void) {
	klog_info("Enabling FPGA bridges...\n");

	//request_mem_region fails
	//already reserved: ffd05000-ffd050ff : /sopc@0/rstmgr@0xffd05000 (cf /proc/iomem)
	//continue without reserving memory region...
	/*
	struct resource* i_rst_mgr_rstmgr_region = request_mem_region(BRGMODRST_ADDR, 1, "brgmodrst");
	*/

	void* brgmodrst_ptr = ioremap(BRGMODRST_ADDR, 1);
	if (brgmodrst_ptr == 0) {
		klog_error("Error while remapping brgmodrst (0x%x)\n", BRGMODRST_ADDR);
		return false;
	}

	int brgmodrst = ioread32(brgmodrst_ptr);
	brgmodrst = brgmodrst & BRGMODRST_MASK;
	iowrite32(brgmodrst, brgmodrst_ptr);

	iounmap(brgmodrst_ptr);

	/*
	release_mem_region(BRGMODRST_ADDR, 1);
	*/

	klog_info("Enabling FPGA bridges: OK\n");
	return true;
}