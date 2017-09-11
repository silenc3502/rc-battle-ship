/***********************************************/
// 
//	2017. 06. 08
//	Xilinx FPGA Zynq API Descriptions
//
/***********************************************/

void GpioDisableIntr	(	INTC * 	IntcInstancePtr,
XGpio * 	InstancePtr,
u16 	IntrId,
u16 	IntrMask 
)		
// This function disables the interrupts for the GPIO.

// Parameters
// 	IntcInstancePtr	is a pointer to the Interrupt Controller driver Instance
// 	InstancePtr	is a pointer to the GPIO driver Instance
// 	IntrId	is XPAR_<INTC_instance>_<GPIO_instance>_VEC value from xparameters.h
// 	IntrMask	is the GPIO channel mask
// 	Returns
// 	None