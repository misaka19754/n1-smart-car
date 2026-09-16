/*
 * Subject_2.c
 *
 *  Created on: 2026��5��4��
 *      Author: misaka19754
 *
 *  �ߵ��ط�ģʽ��
 *    1. �ֶ��Ƴ�¼�ư���·����Body_keep + guandao_record��
 *    2. �ط�ʱ guandao_load() �Զ����� Taget_angle
 *    3. ת��ֱ�Ӹ��� Taget_angle��guandao_load �ڲ��������ݱƽ���
 *    4. �����Զ�ͣ��
 */

#include "zf_common_headfile.h"


void Body_ctrl_2(void)
{
    static bool once = false;     //��ֻ֤��ֵһ��
    if (!once)
    {
      balance_mode_parameter(2);   //�����Ŀ2����

      /* �ط�ʱĿ��Ǵӵ�ǰ yaw ��ʼ��guandao_load ��������·������ */
      if(gd_mode == guandao_load_mode && guandao_index > 0)
      {
          Taget_angle = roll_balance_cascade.posture_value.yaw;
      }

      once = true;
    }


    if(sys_times%5==0)//�ǶȻ�
    {
        pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);
    }

    if(sys_times%20==0)//ת��
    {
        /* Taget_angle �� guandao_load() �� ISR ���Զ��������£�����Ҫ���� StepApproach */
        pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw), 0);
    }


     Steer_set(SERVO_MOTOR_MID + roll_balance_cascade.angle_cycle.out);

     /* ·���յ��⣬�Զ�ͣ�� */
     if(guandao_new_cnt >= guandao_index)
     {
//         CYT2_S_motor_loop_ctrl(0);
         CYT2_S_motor_ctrl(0);
     }
     else
     {
//         CYT2_S_motor_loop_ctrl(33);//�ջ�
         CYT2_S_motor_ctrl(750);//����
     }

}


/*
 * LCD ������ʾ �� �� ips200 ����ʾ�ߵ����ٹؼ�����
 */
void Balance_2_text(void)
{
    float yaw_err = AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw);
    float yaw_ref = 0;
    if(guandao_new_cnt < guandao_index) yaw_ref = Yaw_Record_f[guandao_new_cnt];

    ips_show_string(8*0,  16*0, "YAW:");      ips_show_float(8*10, 16*0, roll_balance_cascade.posture_value.yaw, 3, 6);
    ips_show_string(8*0,  16*1, "T_A:");       ips_show_float(8*10, 16*1, Taget_angle, 3, 6);
    ips_show_string(8*0,  16*2, "Y_ERR:");     ips_show_float(8*10, 16*2, yaw_err, 3, 6);
    ips_show_string(8*0,  16*3, "c_err:");     ips_show_float(8*10, 16*3, c_error, 3, 6);
    ips_show_string(8*0,  16*4, "Y_ref:");     ips_show_float(8*10, 16*4, yaw_ref, 3, 6);
    ips_show_string(8*0,  16*5, "idx:");       ips_show_uint(8*10, 16*5, guandao_new_cnt, 5);
    ips_show_string(8*0,  16*6, "total:");     ips_show_uint(8*10, 16*6, guandao_index, 5);
    ips_show_string(8*0,  16*7, "X:");         ips_show_float(8*10, 16*7, data_x.f, 3, 6);
    ips_show_string(8*0,  16*8, "Y:");         ips_show_float(8*10, 16*8, data_y.f, 3, 6);
    ips_show_string(8*0,  16*9, "G_out:");     ips_show_float(8*10, 16*9, roll_balance_cascade.angle_cycle.out, 3, 6);

    /* �����ʾ����״̬ */
    if(guandao_new_cnt >= guandao_index && guandao_index > 0)
        ips_show_string(8*0, 16*11, " === FINISHED ===");
    else if(gd_mode == guandao_load_mode)
        ips_show_string(8*0, 16*11, " >>> TRACKING <<<");
    else
        ips_show_string(8*0, 16*11, " --- IDLE ---");
}


/*
 * ���ڵ������ �� ÿ 100ms ��ӡһ�йߵ���������
 * �ڴ��������в鿴������¼������ͼ������
 */
void Balance_2_printf(void)
{
    static uint32 print_tick = 0;
    print_tick++;

    if(print_tick < 100) return;  // 100ms ���һ�� (sys_times 1ms)
    print_tick = 0;

    float yaw     = roll_balance_cascade.posture_value.yaw;
    float yaw_ref = 0;
    if(guandao_new_cnt < guandao_index) yaw_ref = Yaw_Record_f[guandao_new_cnt];

    printf("INS|idx=%4d/%d|yaw=%+7.2f|T_A=%+7.2f|c_err=%+6.3f|yaw_ref=%+7.2f|X=%+6.3f|Y=%+6.3f\r\n",
           guandao_new_cnt, guandao_index,
           yaw, Taget_angle, c_error, yaw_ref,
           data_x.f, data_y.f);
}
