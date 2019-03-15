#include "mbed.h"
//#include "I2C.h"
#include "gpio_api.h"
#include "DX8_API.h"


//�������� 1:ʹ�����ģ��I2C  

static gpio_t dx8I2C_SCL;
static gpio_t dx8I2C_SDA;

typedef unsigned char  u8;
typedef __IO uint32_t  vu32;

static __inline void TWI_SCL_0(void)        { gpio_write(&dx8I2C_SCL,0); } 
static __inline void TWI_SCL_1(void)        { gpio_write(&dx8I2C_SCL,1); } 
static __inline void TWI_SDA_0(void)        { gpio_write(&dx8I2C_SDA,0); } 
static __inline void TWI_SDA_1(void)        { gpio_write(&dx8I2C_SDA,1); } 
static __inline u8   TWI_SDA_STATE(void)    { return gpio_read(&dx8I2C_SDA); }

static __inline void TWI_SDA_INPUT(void)    { gpio_dir(&dx8I2C_SDA,PIN_INPUT); }
static __inline void TWI_SDA_OUTPUT(void)   { gpio_dir(&dx8I2C_SDA,PIN_OUTPUT); }
  
/******************************************************************************* 
 * ��������:TWI_Initialize                                                                      
 * ��    ��:I2C��ʼ������                                                                      
 *                                                                                
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:                                                                      
 * �޸�����:2010��6��8��                                                                     
 *******************************************************************************/  
void dx8_Init(void)  
{ 

/* I2C Pins: sda, scl */
//mbed::I2C i2c(GPIO_PIN22, GPIO_PIN23);
//static mbed::I2C i2c(PD_0, PD_1);


#if 0
  gpio_init(&dx8I2C_SCL,PD_0);  
  gpio_init(&dx8I2C_SDA,PD_1);
#else
  gpio_init(&dx8I2C_SCL,PD_1);  
  gpio_init(&dx8I2C_SDA,PD_0);
#endif


  TWI_SCL_1();
	TWI_SDA_1();
  gpio_dir(&dx8I2C_SCL,PIN_OUTPUT);
  gpio_dir(&dx8I2C_SDA,PIN_OUTPUT);
	TWI_SCL_1();
	TWI_SDA_1();
} 

/******************************************************************************* 
 * ��������:TWI_Delay                                                                      
 * ��    ��:��ʱ����                                                                      
 *                                                                                
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:                                                                      
 * �޸�����:2010��6��8��                                                                     
 *******************************************************************************/  
static void TWI_NOP(void)  
{  
		vu32 i, j;  
		vu32 sum = 0;  
		i = 2;  
		while(i--)  
		{  
			 for (j = 0; j < 2; j++)  
			 sum += i;   
		}  
		sum = i;  
}
  
/******************************************************************************* 
 * ��������:TWI_START                                                                      
 * ��    ��:��������                                                                      
 *                                                                                
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:��                                                                      
 * ��    ��:                                                                      
 * �޸�����:2010��6��8��                                                                     
 *******************************************************************************/  
static void TWI_START(void)  
{   
		TWI_SDA_1();   
		TWI_NOP();  
			 
		TWI_SCL_1();   
		TWI_NOP();      

		TWI_SDA_0();  
		TWI_NOP();  
			
		TWI_SCL_0();    
		TWI_NOP(); 
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_STOP  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_STOP(void)  
{  
		TWI_SDA_0();   
		TWI_NOP();  
			 
		TWI_SCL_1();   
		TWI_NOP();      

		TWI_SDA_1();  
		TWI_NOP();   
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendACK  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_SendACK(void)  
{  
		TWI_SDA_0();  
		TWI_NOP();  
		TWI_SCL_1();  
		TWI_NOP();  
		TWI_SCL_0();   
		TWI_NOP();  
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendNACK  
 */  
/* --------------------------------------------------------------------------*/  
static void TWI_SendNACK(void)  
{  
		TWI_SDA_1();  
		TWI_NOP();  
		TWI_SCL_1();  
		TWI_NOP();
		TWI_SCL_0();  
		TWI_NOP();    
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_SendByte  
 *  
 * @Param: Data 
 *  
 * @Returns:    
 */  
/* --------------------------------------------------------------------------*/  
static u8 TWI_SendByte(u8 Data)  
{  
		u8 i,ret;

		for(i = 0; i < 8; i++)
		{    
				//---------���ݽ���----------  
				if(Data & 0x80)  
				{  
						TWI_SDA_1(); 
				}  
				else  
				{  
						TWI_SDA_0();  
				}   
				Data<<=1;  
				TWI_NOP(); 

				//---���ݽ�������һ����ʱ----    
				//----����һ��������[������]   
				TWI_SCL_1();  
				TWI_NOP();  
				TWI_SCL_0();  
				TWI_NOP();//��ʱ,��ֹSCL��û��ɵ�ʱ�ı�SDA,�Ӷ�����START/STOP�ź�  
				//---------------------------     
		} 

		//���մӻ���Ӧ��   
		TWI_SDA_INPUT();
		TWI_NOP();
		
		TWI_SCL_1();  
		TWI_NOP();
		ret = TWI_SDA_STATE();
		TWI_NOP();
		TWI_SCL_0();
		TWI_NOP();
	
		TWI_NOP();
		TWI_NOP();
		TWI_SDA_OUTPUT();			
		TWI_NOP();		
				  
    if (ret)
		{
				return 1; 
		}
		else  
		{  
				return 0;    
		}      
}  
  
/* --------------------------------------------------------------------------*/  
/**  
 * @Brief:  TWI_ReceiveByte  
 *  
 * @Returns:    
 */  
/* --------------------------------------------------------------------------*/  
static u8 TWI_ReceiveByte(void)  
{  
		u8 i,Dat;

		TWI_SDA_INPUT();
    TWI_NOP();	
  
		Dat = 0;  
		for(i = 0; i < 8; i++)  
		{  
				TWI_SCL_1();//����ʱ��������[������],�ôӻ�׼��������   
				TWI_NOP();   
				Dat<<=1;
			
				if(TWI_SDA_STATE()) //������״̬  
				{  
				 Dat |= 0x01;   
				}
				
				TWI_SCL_0();//׼�����ٴν�������    
				TWI_NOP();//�ȴ�����׼����           
		}

		TWI_NOP();
		TWI_NOP();
		TWI_SDA_OUTPUT();
		TWI_NOP();
 
		return Dat;  
}  

unsigned char dxif_transfer(unsigned char *buf, unsigned short len)
{
		unsigned short i;
		
		dx8_Init();

		// Start
		TWI_START();

		// slave address
		if (0 != TWI_SendByte(buf[0])) 
		{
				TWI_STOP();
				return 1;
		}

		// Data
		if (buf[0] & 0x01)  // i2c read
		{
				for (i = 1; i < len; i++) 
				{
					buf[i] = TWI_ReceiveByte();
					if (i == (len - 1)) 
					{
							TWI_SendNACK();
					}
					else 
					{
							TWI_SendACK();
					}
				}
		}
		else  // i2c write
		{		
				for (i = 1; i < len; i++) 
				{
						if (0 != TWI_SendByte(buf[i]))
						{
								TWI_STOP();
								return 1;
						}
				}
		}

		// Stop
		TWI_STOP();

		return 0;
}
