
#include "zf_common_headfile.h"
#pragma section all "cpu0_dsram"
// 将本语句与#pragma section all restore语句之间的全局变量都放在CPU0的RAM中


// **************************** 代码区域 ****************************
int core0_main(void)
{
    clock_init();                   // 获取时钟频率<务必保留>
    debug_init();                   // 初始化默认调试串口


    Buzzer_init();                                      //蜂鸣器初始化
    Key_init();                                         //按键初始化
    Steer_init();                                       //舵机初始化
    small_driver_uart_init();                           //无刷驱动初始化
////    QUD_encoder_init();                                 //正交编码器初始化
    ips_init(IPS200_TYPE_SPI);                          //显示屏初始化
    Imu_init();                                         //IMU初始化
////    uart_receiver_init();                               //sbus接收机初始化
    balance_cascade_init();                             //平衡及陀螺仪参数初始化
    guandao_Init();                                     //惯导初始化

    Buzzer_check(50);                                       //外设初始化成功

    system_delay_ms(1000);

    pit_ms_init(CCU60_CH0,1);


    Buzzer_check(300);                                       //定时器初始化成功


    // 此处编写用户代码 例如外设初始化代码等
    cpu_wait_event_ready();         // 等待所有核心初始化完毕
    while (TRUE)
    {
        // 此处编写需要循环执行的代码
//        Motor_text();
//        Encoder_text();
//        Steer_text();
//        IMU_text();
        Balance_1_text();

//        Ins_text();

        /* 惯导回放时显示完整跟踪数据，否则显示普通 INS 日志 */
//        if(Mode_chage == 2)
//        {
//            Balance_2_text();       // LCD 显示惯导跟踪调试数据
//            Balance_2_printf();     // 串口输出惯导跟踪数据
//        }
//        else
//        {
//            INS_log();
//        }

//        New_ctrl();

        Menu();


//        printf("%d,%f\r\n",-motor_value.receive_left_speed_data,roll_balance_cascade.speed_cycle.out);
//         printf("%d,%f\r\n",-motor_value.receive_left_speed_data,guandao_lucheng);

//        CYT2_S_motor_ctrl(1);
        // 此处编写需要循环执行的代码
    }
}


IFX_INTERRUPT(cc60_pit_ch0_isr, 0, CCU6_0_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);                        // 开启中断嵌套
    pit_clear_flag(CCU60_CH0);



    sys_times ++;                                      // 系统计时自增

    if(sys_times%5==0)
    {
        Key_scan();                                    //按键扫描
    }

    Imu_attitude_scan();                               //姿态解算

//    QUD_encoder_pulse_get();                           //编码器数据采集

    INS_data_get();                                  //惯导数据采集

    guandao_task();                                  //惯导读取与复现





    if(CTRL_flag==0)//默认无遥控器执行正常程序
    {
        Sub_select(SUB_flag);
    }


    if(Mode_chage==1)//车头随动模式用以惯导推车
    {
        Body_keep();
    }

//    static bool once = false;     //保证只赋值一次
//    if (!once)
//    {
//      balance_mode_parameter(1);   //分配科目一参数
//      once = true;
//    }


//    CYT2_S_motor_loop_ctrl(798);
//    CYT2_S_motor_ctrl(1500);
}



#pragma section all restore
// **************************** 代码区域 ****************************
