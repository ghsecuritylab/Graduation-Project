/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2013-07-12     aozima       update for auto initial.
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>
#include <pwm.h>

#ifdef  RT_USING_COMPONENTS_INIT
#include <components.h>
#endif  /* RT_USING_COMPONENTS_INIT */

#ifdef RT_USING_DFS
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/driver.h>
#include <rtgui/calibration.h>
#endif

#include "led.h"
#include "sys.h"
#include "wifi.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[ 512 ];
static struct rt_thread led_thread;
static void led_thread_entry(void* parameter)
{
    unsigned int count=0;

    rt_hw_led_init();

    while (1)
    {
        /* led1 on */
#ifndef RT_USING_FINSH
        rt_kprintf("led on, count : %d\r\n",count);
#endif
        count++;
		
				LED1 = 0;
				LED2 = 0;
				LED3 = 0;
        rt_thread_delay( RT_TICK_PER_SECOND/2 ); /* sleep 0.5 second and switch to other thread */

        /* led1 off */
#ifndef RT_USING_FINSH
        rt_kprintf("led off\r\n");
#endif
				LED1 = 1;
				LED2 = 1;
				LED3 = 1;
        rt_thread_delay( RT_TICK_PER_SECOND/2 );
    }
}

#ifdef RT_USING_RTGUI
rt_bool_t cali_setup(void)
{
    rt_kprintf("cali setup entered\n");
    return RT_FALSE;
}

void cali_store(struct calibration_data *data)
{
    rt_kprintf("cali finished (%d, %d), (%d, %d)\n",
               data->min_x,
               data->max_x,
               data->min_y,
               data->max_y);
}
#endif /* RT_USING_RTGUI */

void rt_init_thread_entry(void* parameter)
{
#ifdef RT_USING_COMPONENTS_INIT
    /* initialization RT-Thread Components */
    rt_components_init();
#endif

    /* Filesystem Initialization */
#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
    /* mount sd card fat partition 1 as root directory */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("File System initialized!\n");
    }
    else
        rt_kprintf("File System initialzation failed!\n");
#endif  /* RT_USING_DFS */

#ifdef RT_USING_RTGUI
    {
        extern void rt_hw_lcd_init();
        extern void rtgui_touch_hw_init(void);

        rt_device_t lcd;

        /* init lcd */
        rt_hw_lcd_init();

        /* init touch panel */
        rtgui_touch_hw_init();

        /* find lcd device */
        lcd = rt_device_find("lcd");

        /* set lcd device as rtgui graphic driver */
        rtgui_graphic_set_device(lcd);

#ifndef RT_USING_COMPONENTS_INIT
        /* init rtgui system server */
        rtgui_system_server_init();
#endif

        calibration_set_restore(cali_setup);
        calibration_set_after(cali_store);
        calibration_init();
    }
#endif /* #ifdef RT_USING_RTGUI */
}

/* thread_can */
ALIGN(RT_ALIGN_SIZE)
static char thread_can_stack[512];
static struct rt_thread thread_can_handle;
void rt_entry_thread_can(void* parameter)
{
	extern int thread_can(void);
	thread_can();
}

ALIGN(RT_ALIGN_SIZE)
static char mpu9250_statck[512];
static struct rt_thread mpu9250_thread;
extern void mpu9250_thread_entry(void* parameter);

ALIGN(RT_ALIGN_SIZE)
static char thread_wifi_stack[1024];
struct rt_thread wifi_thread;
extern void wifi_thread_entry(void *parameter);
int rt_application_init(void)
{
    rt_thread_t init_thread;

    rt_err_t result;

    /* init led thread */
    result = rt_thread_init(&led_thread,
                            "led",
                            led_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&led_stack[0],
                            sizeof(led_stack),
                            8,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&led_thread);
    }
	//------- can
    result = rt_thread_init(&thread_can_handle,
														 "can",
														 rt_entry_thread_can,
														 RT_NULL,
														 &thread_can_stack[0],
														 sizeof(thread_can_stack),8,20);
		if (result == RT_EOK)
		{
				rt_thread_startup(&thread_can_handle);			
		}

	//------ mpu9250-------------------
    result = rt_thread_init(&mpu9250_thread,
														 "mpu9250",
														 mpu9250_thread_entry,
														 RT_NULL,
														 &mpu9250_statck[0],
														 sizeof(mpu9250_statck),8,20);
		if (result == RT_EOK)
		{
				rt_thread_startup(&mpu9250_thread);			
		}
		//----- wifi esp8266-------------
//	  result = rt_thread_init(&wifi_thread,
//                            "wifi_thread",
//                            wifi_thread_entry, RT_NULL,
//                             (rt_uint8_t*)&thread_wifi_stack[0], sizeof(thread_wifi_stack),
//                            8, 5);
//														
//    if (result == RT_EOK)
//		{
//        rt_thread_startup(&wifi_thread);
//		}
		//-------------------------------
		 MotorPWMInit();
     PWMTimerInit();
#if (RT_THREAD_PRIORITY_MAX == 32)
    init_thread = rt_thread_create("init",
                                   rt_init_thread_entry, RT_NULL,
                                   512, 8, 20);
#else
    init_thread = rt_thread_create("init",
                                   rt_init_thread_entry, RT_NULL,
                                   512, 80, 20);
#endif

    if (init_thread != RT_NULL)
        rt_thread_startup(init_thread);

    return 0;
}

/*@}*/
