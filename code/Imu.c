/*
 * Imu.c
 *
 *  Created on: 2026��4��28��
 *      Author: misaka19754
 */

#include "zf_common_headfile.h"

cascade_value_struct roll_balance_cascade;          // ����ƽ����Ʋ����ṹ��
cascade_value_struct roll_balance_cascade_resave;   // ����ƽ����Ʋ����ṹ���ʼ����
cascade_value_struct pitch_balance_cascade;         // ���ƽ����������ṹ��
cascade_value_struct pitch_balance_cascade_resave;  // ���ƽ����������ṹ���ʼ����

// �������    ��������ֵ�ķ����У����Ƽ��㣩
// ���ز���   float - �����нǶ�ֵ����λ���ȣ�
// ʹ��ʾ��   float angle = arctan1(1.0f); // ���� tan(45��) �ķ����У����� 45.0 ��
// ��ע��Ϣ   ���÷ֶν��ƹ�ʽ���㣬�����㣬�����ڶԾ���Ҫ�󲻸ߵĳ���
static float arctan1(float tan)
{
    // �ֶμ��㷴���н���ֵ������ tan �ľ���ֵ�Ƿ���� 1 ѡ��ͬ��ʽ
    float angle = (func_abs(tan) > 1.0f) ? 90.0f - (func_abs(1.0f / tan)) * (45.0f - (func_abs(1.0f / tan) - 1.0f) * (14.0f + 3.83f * func_abs(1.0f / tan))) :
                                          func_abs(tan) * (45.0f - (func_abs(tan) - 1.0f) * (14.0f + 3.83f * func_abs(tan)));
    return (tan > 0) ? angle : -angle;  // ���� tan ����ȷ���Ƕ�����
}

// �������    �����ά����ķ����У����� atan2�����ؽǶȣ�
// ���ز���   float - �Ƕ�ֵ����λ���ȣ���Χ -180��~180�㣩
// ʹ��ʾ��   float angle = arctan2(1.0f, 1.0f); // ���� (1,1) ��ĽǶȣ����� 45.0 ��
// ��ע��Ϣ   ���� arctan1 ʵ�֣��������������������x �� y Ϊ 0 ʱ��
static float arctan2(float x, float y)
{
    float tan, angle;

    if (x == 0 && y == 0) return 0;    // ԭ�����⴦�������� 0 ��
    if (x == 0)                         // x Ϊ 0 ʱ���Ƕ�Ϊ ��90 ��
    {
        if (y > 0) return 90;
        else return -90;
    }
    if (y == 0)                         // y Ϊ 0 ʱ���Ƕ�Ϊ 0 �� -180 ��
    {
        if (x > 0) return 0;
        else return -180.0f;
    }
    tan = y / x;                        // ��������ֵ
    angle = arctan1(tan);               // ���� arctan1 ��������Ƕ�

    if (x < 0 && angle > 0)             // ���� x �ķ��ŵ����Ƕȣ�ȷ����Χ��ȷ
    {
        angle -= 180.0f;
    }
    else if (x < 0 && angle < 0)
    {
        angle += 180.0f;
    }
    return angle;
}

// �������    ��������ֵ�ķ����ң����Ƽ��㣩
// ���ز���   float - �����ҽǶ�ֵ����λ���ȣ�
// ʹ��ʾ��   float angle = arcsin(1.0f); // ���� sin(90��) �ķ����ң����� 90.0 ��
// ��ע��Ϣ   ���� arctan1 ʵ�֣����ú��ʽ arcsin(x) = arctan(x / sqrt(1 - x2))
static float arcsin(float i)
{
    return arctan1(i / sqrt(1 - i * i)); // ת��Ϊ�����м���
}

// �������    ���ٶȼ����ݵ�ͨ�˲�
// ���ز���   void
// ʹ��ʾ��   acc_lowpass_filter(&ax, &ay, &az, &fax, &fay, &faz, 0.8f);
// ��ע��Ϣ   һ�׵�ͨ�˲�����ʽ��filtered = alpha * filtered + (1 - alpha) * raw, alpha Խ���˲�Խǿ
static void acc_lowpass_filter(float *raw_x, float *raw_y, float *raw_z, float *filtered_x, float *filtered_y, float *filtered_z, float alpha)
{
    *filtered_x = alpha * *filtered_x + (1 - alpha) * *raw_x;  // X ���˲�����
    *filtered_y = alpha * *filtered_y + (1 - alpha) * *raw_y;  // Y ���˲�����
    *filtered_z = alpha * *filtered_z + (1 - alpha) * *raw_z;  // Z ���˲�����
}

// �������    ���ٶȼ����ݹ�һ������λ����
// ���ز���   void
// ʹ��ʾ��   acc_normalize(&ax, &ay, &az);
// ��ע��Ϣ   �����ٶ�������һ��Ϊ��λ��������ģ����Сʱ��<0.1����Ĭ����Ϊ (0,0,1)
static void acc_normalize(float *ax, float *ay, float *az)
{
    float norm = sqrt(*ax * *ax + *ay * *ay + *az * *az);    // ������ٶ�����ģ��
    if (norm < 0.1f)                                        // ģ����С����Ϊ��Ч���ݣ���Ĭ��ֵ
    {
        *ax = 0.0f;
        *ay = 0.0f;
        *az = 1.0f;
    }
    else                                                    // ģ����Ч����һ��
    {
        *ax /= norm;
        *ay /= norm;
        *az /= norm;
    }
}

// �������    �ж��豸�Ƿ��ھ�ֹ״̬
// ���ز���   bool - ��ֹ״̬���� true���˶�״̬���� false
// ʹ��ʾ��   bool static_flag = is_static_state(ax_g, ay_g, az_g);
// ��ע��Ϣ   ͨ�����ٶ�ģ���жϣ���ֹʱ���ٶȽӽ� 1g����ģ���� 0.9~1.1g ��Χ����Ϊ��ֹ
static bool is_static_state(float ax_g, float ay_g, float az_g)
{
    float norm = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);  // ������ٶ�ģ����g Ϊ��λ��
    return (norm >= 0.9f && norm <= 1.1f) ? true : false;       // �ж��Ƿ��ھ�ֹ��Χ��
}

// �������    ��Ԫ������̬�Ǽ���
// ���ز���   void
// ʹ��ʾ��   quaternion_module_calculate(&quaternion, 0.001f); // �� 1 ms ���ڵ���
// ��ע��Ϣ   �ں������Ǻͼ��ٶȼ����ݣ�ͨ���ݶ��½���������Ԫ����������ת�������̬��
//            ������� cycle Ϊ�������ڣ���λ���룩�����ȶ������Ա�֤���㾫��
void quaternion_module_calculate(cascade_value_struct *cascade_value)
{
    static float first_count_time = 0;  // �״μ���ʱ������������ڿ���������
    float length;                       // ��Ԫ��ģ�������ڹ�һ����
    float x, y, z;                      // �����ǽ��ٶȣ�����/�룩

    // ����������ת����ԭʼ���� -> (��/s) -> ����/�루�ȳ��� 10 �ٳ� 10 �����˲���
    x = (float)(GYRO_DATA_X / 10 * 10) / GYRO_TRANSITION_FACTOR * 0.01745329f;  // 0.01745329 Ϊ��ת����ϵ������/180��
    y = (float)(GYRO_DATA_Y / 10 * 10) / GYRO_TRANSITION_FACTOR * 0.01745329f;
    z = (float)(GYRO_DATA_Z / 10 * 10) / GYRO_TRANSITION_FACTOR * 0.01745329f;

    // ���ٶȼ�����ת����ԭʼ���� -> g Ϊ��λ��1g �� 9.8 m/s2��
    float ax_g = (float)ACC_DATA_X / ACC_TRANSITION_FACTOR;
    float ay_g = (float)ACC_DATA_Y / ACC_TRANSITION_FACTOR;
    float az_g = (float)ACC_DATA_Z / ACC_TRANSITION_FACTOR;

    bool static_state = is_static_state(ax_g, ay_g, az_g);  // �ж��豸�Ƿ�ֹ
    float acc_alpha = static_state ? 0.8f : 0.5f;           // ��ֹʱ�˲�ϵ������ƽ��Ч�����ã�

    // ���ٶȼ����ݵ�ͨ�˲�
    acc_lowpass_filter(&ax_g, &ay_g, &az_g,
                       &cascade_value->quaternion.pro.acc_filtered[0],
                       &cascade_value->quaternion.pro.acc_filtered[1],
                       &cascade_value->quaternion.pro.acc_filtered[2], acc_alpha);

    // ȡ�˲���ļ��ٶ�����
    float ax = cascade_value->quaternion.pro.acc_filtered[0];
    float ay = cascade_value->quaternion.pro.acc_filtered[1];
    float az = cascade_value->quaternion.pro.acc_filtered[2];

    acc_normalize(&ax, &ay, &az);  // ���ٶ����ݹ�һ��

    // ȡ����ǰ��Ԫ�� (w, x, y, z)
    float q0 = cascade_value->quaternion.pro.qua[0];
    float q1 = cascade_value->quaternion.pro.qua[1];
    float q2 = cascade_value->quaternion.pro.qua[2];
    float q3 = cascade_value->quaternion.pro.qua[3];

    // ���ݵ�ǰ��Ԫ���������������ڻ�������ϵ�е�ͶӰ����������ٶȼ����ݶԱȣ�
    float gx = 2 * (q1 * q3 - q0 * q2);
    float gy = 2 * (q0 * q1 + q2 * q3);
    float gz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    // ������ٶȼ�������ͶӰ�����ݶ���
    float ex = ay * gz - az * gy;
    float ey = az * gx - ax * gz;
    float ez = ax * gy - ay * gx;

    // ���ݾ�ֹ״̬����У��ϵ�����˶�ʱ����У������������������KI��˥������֤����У������
    float kp = static_state ? cascade_value->posture_value.correct_kp : cascade_value->posture_value.correct_kp * 0.8f;
    float ki = cascade_value->posture_value.correct_ki;

    // �״μ����ǰ 0.1 �룬ʹ�ô����ϵ����������
//    if(first_count_time < 0.1f)
//    {
//        first_count_time += cascade_value->posture_value.call_cycle;  // �ۼ�ʱ��
//        kp = 10.0f;  // ǿ����У������������
//    }

    if(first_count_time < 0.1f)
    {
        first_count_time += cascade_value->posture_value.call_cycle;  // �ۼ�ʱ��
        kp = 100.0f;  // ǿ����У������������
    }

    // ��̬ʱ�������Ƚ�Ϊ1/10
    float integral_gain = static_state ? 1.0f : 0.1f;

    cascade_value->quaternion.parameter.acc_err[0] += (ex * cascade_value->posture_value.call_cycle) * integral_gain;
    cascade_value->quaternion.parameter.acc_err[1] += (ey * cascade_value->posture_value.call_cycle) * integral_gain;
    cascade_value->quaternion.parameter.acc_err[2] += (ez * cascade_value->posture_value.call_cycle) * integral_gain;

    // �����޷�����ֹ����
    float acc_err_limit = 1.0f;
    cascade_value->quaternion.parameter.acc_err[0] = (cascade_value->quaternion.parameter.acc_err[0] > acc_err_limit) ? acc_err_limit :
                                                      (cascade_value->quaternion.parameter.acc_err[0] < -acc_err_limit) ? -acc_err_limit :
                                                      cascade_value->quaternion.parameter.acc_err[0];
    cascade_value->quaternion.parameter.acc_err[1] = (cascade_value->quaternion.parameter.acc_err[1] > acc_err_limit) ? acc_err_limit :
                                                      (cascade_value->quaternion.parameter.acc_err[1] < -acc_err_limit) ? -acc_err_limit :
                                                      cascade_value->quaternion.parameter.acc_err[1];
    cascade_value->quaternion.parameter.acc_err[2] = (cascade_value->quaternion.parameter.acc_err[2] > acc_err_limit) ? acc_err_limit :
                                                      (cascade_value->quaternion.parameter.acc_err[2] < -acc_err_limit) ? -acc_err_limit :
                                                      cascade_value->quaternion.parameter.acc_err[2];

    // �����У�����������ݣ�����+����У����
    x += kp * ex + ki * cascade_value->quaternion.parameter.acc_err[0];
    y += kp * ey + ki * cascade_value->quaternion.parameter.acc_err[1];
    z += kp * ez + ki * cascade_value->quaternion.parameter.acc_err[2];

    // ��Ԫ��΢�ַ��̸��£����������ǽ��ٶȣ�
    cascade_value->quaternion.pro.qua[0] += ((-q1 * x - q2 * y - q3 * z) * cascade_value->posture_value.call_cycle / 2.0f);
    cascade_value->quaternion.pro.qua[1] += (( q0 * x + q2 * z - q3 * y) * cascade_value->posture_value.call_cycle / 2.0f);
    cascade_value->quaternion.pro.qua[2] += (( q0 * y - q1 * z + q3 * x) * cascade_value->posture_value.call_cycle / 2.0f);
    cascade_value->quaternion.pro.qua[3] += (( q0 * z + q1 * y - q2 * x) * cascade_value->posture_value.call_cycle / 2.0f);

    // ȡ����ǰ��Ԫ�� (w, x, y, z)
    q0 = cascade_value->quaternion.pro.qua[0];
    q1 = cascade_value->quaternion.pro.qua[1];
    q2 = cascade_value->quaternion.pro.qua[2];
    q3 = cascade_value->quaternion.pro.qua[3];

    // ������Ԫ��������ƽ���������ظ����㣩
    float q0_2 = q0 * q0;
    float q1_2 = q1 * q1;
    float q2_2 = q2 * q2;
    float q3_2 = q3 * q3;

    // ��Ԫ����һ����������ֵƯ�ƣ�
    length = sqrt(q0_2 + q1_2 + q2_2 + q3_2);  // ����ģ��
    if (length > 0.001f)  // ģ����Чʱ�Ź�һ��
    {
        cascade_value->quaternion.pro.qua[0] /= length;
        cascade_value->quaternion.pro.qua[1] /= length;
        cascade_value->quaternion.pro.qua[2] /= length;
        cascade_value->quaternion.pro.qua[3] /= length;
    }

    // ������Ԫ��������ת�������ں�����̬�Ǽ��㣩
    cascade_value->quaternion.data.rot_mat[0][0] = q0_2 + q1_2 - q2_2 - q3_2;
    cascade_value->quaternion.data.rot_mat[0][1] = 2 * (q1 * q2 + q0 * q3);
    cascade_value->quaternion.data.rot_mat[0][2] = 2 * (q1 * q3 - q0 * q2);
    cascade_value->quaternion.data.rot_mat[1][0] = 2 * (q1 * q2 - q0 * q3);
    cascade_value->quaternion.data.rot_mat[1][1] = q0_2 - q1_2 + q2_2 - q3_2;
    cascade_value->quaternion.data.rot_mat[1][2] = 2 * (q2 * q3 + q0 * q1);
    cascade_value->quaternion.data.rot_mat[2][0] = 2 * (q1 * q3 + q0 * q2);
    cascade_value->quaternion.data.rot_mat[2][1] = 2 * (q2 * q3 - q0 * q1);
    cascade_value->quaternion.data.rot_mat[2][2] = q0_2 - q1_2 - q2_2 + q3_2;

//    // ������ת���������̬�ǣ�����ǡ������ǡ�ƫ���ǣ�

// imu660rb
    cascade_value->posture_value.rol = arctan2(cascade_value->quaternion.data.rot_mat[2][2], cascade_value->quaternion.data.rot_mat[1][2]);  // �����
    cascade_value->posture_value.pit = -arcsin(cascade_value->quaternion.data.rot_mat[0][2]);                                            // ������
    cascade_value->posture_value.yaw = arctan2(cascade_value->quaternion.data.rot_mat[0][0], cascade_value->quaternion.data.rot_mat[0][1]);   // ƫ����

////imu660ra
//       cascade_value->posture_value.rol = -arctan2(cascade_value->quaternion.data.rot_mat[2][2], cascade_value->quaternion.data.rot_mat[1][2]);  // �����
//       cascade_value->posture_value.pit = -arcsin(cascade_value->quaternion.data.rot_mat[0][2]);                                            // ������
//       cascade_value->posture_value.yaw = arctan2(cascade_value->quaternion.data.rot_mat[0][0], cascade_value->quaternion.data.rot_mat[0][1]);   // ƫ����
}

// �������    PID�ջ�����
// ����˵��   pid_cycle        PID�����ṹ��
// ����˵��   target           Ŀ��ֵ
// ����˵��   real             ��ǰֵ
// ���ز���   void
// ʹ��ʾ��   pid_control(&roll_balance_cascade.speed_cycle, 0, (left_motor.encoder_data + right_motor.encoder_data) / 2);
// ��ע��Ϣ
void pid_control (pid_cycle_struct *pid_cycle, float target, float real)
{
    float    proportion_value    = 0;          // ������
    float    differential_value  = 0;          // ΢����

    proportion_value = target - real;          // ������ = Ŀ��ֵ - ʵ��ֵ



    pid_cycle->i_value += (proportion_value * pid_cycle->i_value_pro);  // ������ = ������ + ������ * ���̶ֳ�

    pid_cycle->i_value = func_limit_ab(pid_cycle->i_value, -pid_cycle->i_value_max, pid_cycle->i_value_max);  // �������޷�

    differential_value = proportion_value - pid_cycle->p_value_last;  // ΢���� = ������ - ��һ�α�����

    pid_cycle->out = (pid_cycle->p * proportion_value + pid_cycle->i * pid_cycle->i_value + pid_cycle->d * differential_value);  // PID���

    pid_cycle->out = func_limit_ab(pid_cycle->out, -pid_cycle->out_max, pid_cycle->out_max);  // PID����޷�

    pid_cycle->p_value_last = proportion_value;        // ���������
}

// �������    PID�ջ�����(����ʽ)
// ����˵��   pid_cycle        PID�����ṹ��
// ����˵��   target           Ŀ��ֵ
// ����˵��   real             ��ǰֵ
// ���ز���   void
// ʹ��ʾ��   pid_control_incremental(&roll_balance_cascade.speed_cycle, 0, (left_motor.encoder_data + right_motor.encoder_data) / 2);
// ��ע��Ϣ
void pid_control_incremental (pid_cycle_struct *pid_cycle, float target, float real)
{
    float    proportion_value    = 0,          // ������
             differential_value  = 0;          // ΢����

    pid_cycle->i_value = target - real;          // ������ = Ŀ��ֵ - ʵ��ֵ  ����ʽPID P ---> I

    differential_value = proportion_value - 2 * pid_cycle->incremental_data[0] - pid_cycle->incremental_data[1];  // ΢���� ����ʽPID I ---> D

    proportion_value  = proportion_value - pid_cycle->incremental_data[0];  // ������ ����ʽPID D ---> P

    pid_cycle->incremental_data[1] = pid_cycle->incremental_data[0];          // ����ʽPID �������

    pid_cycle->incremental_data[0] = proportion_value;

    pid_cycle->out += (pid_cycle->p * proportion_value + pid_cycle->i * pid_cycle->i_value + pid_cycle->d * differential_value);  // PID���

    pid_cycle->out = func_limit_ab(pid_cycle->out, -pid_cycle->out_max, pid_cycle->out_max);          // PID����޷�
}

// �������    ��ʼ����Ԫ��ģ��
// ���ز���   void
// ʹ��ʾ��   quaternion_module_init(&quaternion);
// ��ע��Ϣ   ��ʼ����Ԫ��Ϊ��λ��Ԫ������̬��Ϊ 0�����ٶ��˲���ʼֵΪ��ǰ���ٶ����ݣ�������̬����ɵ��ô˺���
void quaternion_module_init(cascade_value_struct *cascade_value)
{
    // ��ʼ����Ԫ��Ϊ��λ��Ԫ�� (w=1, x=y=z=0)
    cascade_value->quaternion.pro.qua[0] = 1.0f;
    cascade_value->quaternion.pro.qua[1] = 0.0f;
    cascade_value->quaternion.pro.qua[2] = 0.0f;
    cascade_value->quaternion.pro.qua[3] = 0.0f;

    // ��ʼ����̬��Ϊ 0 ��
    cascade_value->posture_value.yaw = 0.0f;
    cascade_value->posture_value.rol = 0.0f;
    cascade_value->posture_value.pit = 0.0f;

    // ��ʼ�����ٶ��˲�ֵΪ��ǰ���ٶ����ݣ�g Ϊ��λ��
    cascade_value->quaternion.pro.acc_filtered[0] = (float)ACC_DATA_X / ACC_TRANSITION_FACTOR;
    cascade_value->quaternion.pro.acc_filtered[1] = (float)ACC_DATA_Y / ACC_TRANSITION_FACTOR;
    cascade_value->quaternion.pro.acc_filtered[2] = (float)ACC_DATA_Z / ACC_TRANSITION_FACTOR;

    // ��ʼ��������Ϊ 0
    cascade_value->quaternion.parameter.acc_err[0] = 0.0f;
    cascade_value->quaternion.parameter.acc_err[1] = 0.0f;
    cascade_value->quaternion.parameter.acc_err[2] = 0.0f;
}

// �������    ����ƽ����Ƴ�ʼ��
// ���ز���   void
// ʹ��ʾ��   balance_cascade_init();
// ��ע��Ϣ   ��ʼ��ƽ����ƽṹ�������������̬У׼ϵ����PID �����ڲ�����P/I/D���޷��ȣ�
//            �������ʼ״̬�����ݽṹ�壬����ʼ����Ԫ��ģ��
void balance_cascade_init (void)
{
    // ��ʼ������ƽ����Ƶ���̬����
    roll_balance_cascade.posture_value.call_cycle        = 0.001;      // �������� 0.001s (1ms)
    roll_balance_cascade.posture_value.mechanical_zero  = 2.1f;      // ��е����ʼ��Ϊ 0
    roll_balance_cascade.posture_value.correct_kp        = 0.4f;       // ��̬У׼����ϵ�� 0.4
    roll_balance_cascade.posture_value.correct_ki        = 0.015f;     // ��̬У׼����ϵ�� 0.015

    // ��ʼ������ƽ����ƵĽ��ٶȻ� PID �޷�����
//    roll_balance_cascade.angular_speed_cycle.i_value_max     = 1000;      // ���ٶȻ���������
//    roll_balance_cascade.angular_speed_cycle.i_value_pro    = 0.1f;       // ���ٶȻ����ֱ���
//    roll_balance_cascade.angular_speed_cycle.out_max        = 50;      // ���ٶȻ��������
//
//    // ��ʼ������ƽ����ƵĽǶȻ� PID �޷�����
//    roll_balance_cascade.angle_cycle.i_value_max        = 1000;      // �ǶȻ���������
//    roll_balance_cascade.angle_cycle.i_value_pro         = 2.0f;       // �ǶȻ����ֱ���
//    roll_balance_cascade.angle_cycle.out_max            = 10000;      // �ǶȻ��������

//    roll_balance_cascade.angle_cycle.i_value_max        = 1000;      // �ǶȻ���������
//    roll_balance_cascade.angle_cycle.i_value_pro        = 2.0f;       // �ǶȻ����ֱ���
//    roll_balance_cascade.angle_cycle.out_max            = 50;      // �ǶȻ��������

//    roll_balance_cascade.turn_cycle.i_value_max        = 1000;      // ת�򻷻�������
//    roll_balance_cascade.turn_cycle.i_value_pro         = 2.0f;     // ת�򻷻��ֱ���
//    roll_balance_cascade.turn_cycle.out_max            = 180;      //  ת���������



    // ��ʼ������ƽ����Ƶ��ٶȻ� PID �޷�����
    roll_balance_cascade.speed_cycle.i_value_max        = 500;      // �ٶȻ���������
    roll_balance_cascade.speed_cycle.i_value_pro         = 0.005f;   // �ٶȻ����ֱ���
    roll_balance_cascade.speed_cycle.out_max            = 9000;      // �ٶȻ��������

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // ����ƽ����Ƹ��� PID �� P/I/D ϵ��
//    roll_balance_cascade.angular_speed_cycle.p    = 0.008f;      // ���ٶȻ� P
//    roll_balance_cascade.angular_speed_cycle.i    = 0.0f;      // ���ٶȻ� I
//    roll_balance_cascade.angular_speed_cycle.d    = 0.0f;      // ���ٶȻ� D
//
//    roll_balance_cascade.angle_cycle.p    = 150.0f;      // �ǶȻ� P
//    roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
//    roll_balance_cascade.angle_cycle.d    = 0.0f;       // �ǶȻ� D
//
////    roll_balance_cascade.angle_cycle.p    = 10.0f;      // �ǶȻ� P
////    roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
////    roll_balance_cascade.angle_cycle.d    = 0.0f;       // �ǶȻ� D
//
//    roll_balance_cascade.turn_cycle.p     = 0.2f;      // ת�� D
//    roll_balance_cascade.turn_cycle.i     = 0.0f;      // ת�� D
//    roll_balance_cascade.turn_cycle.d     = 0.0f;      // ת�� D
//
////        roll_balance_cascade.turn_cycle.p     = 0.2f;      // ת�� D
////        roll_balance_cascade.turn_cycle.i     = 0.0f;      // ת�� D
////        roll_balance_cascade.turn_cycle.d     = 0.0f;      // ת�� D
//
//
//    roll_balance_cascade.speed_cycle.p    = 20.0f;      // �ٶȻ� P
//    roll_balance_cascade.speed_cycle.i    = 0.0f;      // �ٶȻ� I
//    roll_balance_cascade.speed_cycle.d    = 0.0f;      // �ٶȻ� D


//        roll_balance_cascade.angular_speed_cycle.p    = 0.0f;      // ���ٶȻ� P
//        roll_balance_cascade.angular_speed_cycle.i    = 0.0f;      // ���ٶȻ� I
//        roll_balance_cascade.angular_speed_cycle.d    = 0.0f;      // ���ٶȻ� D
//
//        roll_balance_cascade.angle_cycle.p    = 0.0f;      // �ǶȻ� P
//        roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
//        roll_balance_cascade.angle_cycle.d    = 0.0f;       // �ǶȻ� D
//
//        roll_balance_cascade.turn_cycle.p     = 0.0f;      // ת�� D
//        roll_balance_cascade.turn_cycle.i     = 0.0f;      // ת�� D
//        roll_balance_cascade.turn_cycle.d     = 0.0f;      // ת�� D
//
//        roll_balance_cascade.speed_cycle.p    = 0.0f;      // �ٶȻ� P
//        roll_balance_cascade.speed_cycle.i    = 0.0f;      // �ٶȻ� I
//        roll_balance_cascade.speed_cycle.d    = 0.0f;      // �ٶȻ� D


    //////////////////////////////////////////////////////////////////////////////////////////////////

    // ���温��ƽ����Ƴ�ʼ���������ݽṹ��
    memcpy(&roll_balance_cascade_resave, &roll_balance_cascade, sizeof(roll_balance_cascade_resave));
    // ��ʼ������ƽ����Ƶ���Ԫ��ģ��
    quaternion_module_init(&roll_balance_cascade);

    // ��ʼ�����ƽ����Ƶ���̬����
    pitch_balance_cascade.posture_value.call_cycle        = 0.001;      // �������� 0.001s (1ms)
    pitch_balance_cascade.posture_value.mechanical_zero  = 0.0f;       // ��е����ʼ��Ϊ 0
    pitch_balance_cascade.posture_value.correct_kp        = 0.4f;       // ��̬У׼����ϵ�� 0.4
    pitch_balance_cascade.posture_value.correct_ki        = 0.015f;     // ��̬У׼����ϵ�� 0.015

    // ��ʼ�����ƽ����ƵĽ��ٶȻ� PID �޷�����
    pitch_balance_cascade.angular_speed_cycle.i_value_max     = 1000;      // ���ٶȻ���������
    pitch_balance_cascade.angular_speed_cycle.i_value_pro    = 0.3f;       // ���ٶȻ����ֱ���
    pitch_balance_cascade.angular_speed_cycle.out_max        = 10000;      // ���ٶȻ��������

    // ��ʼ�����ƽ����ƵĽǶȻ� PID �޷�����
    pitch_balance_cascade.angle_cycle.i_value_max        = 300;      // �ǶȻ���������
    pitch_balance_cascade.angle_cycle.i_value_pro         = 0.8f;       // �ǶȻ����ֱ���
    pitch_balance_cascade.angle_cycle.out_max            = 300;      // �ǶȻ��������

    // ��ʼ�����ƽ����Ƶ��ٶȻ� PID �޷�����
    pitch_balance_cascade.speed_cycle.i_value_max        = 4000;      // �ٶȻ���������
    pitch_balance_cascade.speed_cycle.i_value_pro         = 0.05f;   // �ٶȻ����ֱ���
    pitch_balance_cascade.speed_cycle.out_max            = 1500;      // �ٶȻ��������

    // ���ƽ����Ƹ��� PID �� P/I/D ϵ��
    pitch_balance_cascade.angular_speed_cycle.p    = 0.0f;      // ���ٶȻ� P
    pitch_balance_cascade.angular_speed_cycle.i    = 0.0f;      // ���ٶȻ� I
    pitch_balance_cascade.angular_speed_cycle.d    = 0.0f;      // ���ٶȻ� D

    pitch_balance_cascade.angle_cycle.p    = 0.0f;      // �ǶȻ� P
    pitch_balance_cascade.angle_cycle.i    = 1.0f;        // �ǶȻ� I
    pitch_balance_cascade.angle_cycle.d    = 0.0f;       // �ǶȻ� D

    pitch_balance_cascade.speed_cycle.p    = 0.0f;      // �ٶȻ� P
    pitch_balance_cascade.speed_cycle.i    = 0.0f;      // �ٶȻ� I
    pitch_balance_cascade.speed_cycle.d    = 0.0f;      // �ٶȻ� D

    // ������ƽ����Ƴ�ʼ���������ݽṹ��
    memcpy(&pitch_balance_cascade_resave, &pitch_balance_cascade, sizeof(pitch_balance_cascade_resave));
}





void balance_mode_parameter(int Mode_change_flag)
{
    if(Mode_change_flag==1)
    {

        roll_balance_cascade.angular_speed_cycle.i_value_max     = 1000;      // ���ٶȻ���������
        roll_balance_cascade.angular_speed_cycle.i_value_pro    = 0.1f;       // ���ٶȻ����ֱ���
        roll_balance_cascade.angular_speed_cycle.out_max        = 50;      // ���ٶȻ��������

        // ��ʼ������ƽ����ƵĽǶȻ� PID �޷�����
        roll_balance_cascade.angle_cycle.i_value_max        = 1000;      // �ǶȻ���������
        roll_balance_cascade.angle_cycle.i_value_pro         = 2.0f;       // �ǶȻ����ֱ���
        roll_balance_cascade.angle_cycle.out_max            = 10000;      // �ǶȻ��������------------��������������,�н��ٶ�,���ԽǶȻ�ֻ���м����ò���Ϊֱ�����

        roll_balance_cascade.turn_cycle.i_value_max        = 1000;      // ת�򻷻�������
        roll_balance_cascade.turn_cycle.i_value_pro         = 2.0f;     // ת�򻷻��ֱ���
        roll_balance_cascade.turn_cycle.out_max            = 180;      //  ת���������

        // ��ʼ������ƽ����Ƶ��ٶȻ� PID �޷�����
        roll_balance_cascade.speed_cycle.i_value_max        = 500;      // �ٶȻ���������
        roll_balance_cascade.speed_cycle.i_value_pro         = 0.005f;   // �ٶȻ����ֱ���
        roll_balance_cascade.speed_cycle.out_max            = 9000;      // �ٶȻ��������

        // ����ƽ����Ƹ��� PID �� P/I/D ϵ��
        roll_balance_cascade.angular_speed_cycle.p    = 0.008f;      // ���ٶȻ� P
        roll_balance_cascade.angular_speed_cycle.i    = 0.0f;      // ���ٶȻ� I
        roll_balance_cascade.angular_speed_cycle.d    = 0.0f;      // ���ٶȻ� D

        roll_balance_cascade.angle_cycle.p    = 150.0f;      // �ǶȻ� P
        roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
        roll_balance_cascade.angle_cycle.d    = 0.0f;        // �ǶȻ� D

        roll_balance_cascade.turn_cycle.p     = 0.3f;        // ת�� D
        roll_balance_cascade.turn_cycle.i     = 0.0f;        // ת�� D
        roll_balance_cascade.turn_cycle.d     = 0.0f;        // ת�� D

        roll_balance_cascade.speed_cycle.p    = 3.5f;       // �ٶȻ� P
        roll_balance_cascade.speed_cycle.i    = 0.0f;        // �ٶȻ� I
        roll_balance_cascade.speed_cycle.d    = 0.0f;        // �ٶȻ� D
    }
    else if(Mode_change_flag==2)
    {
        // ��ʼ������ƽ����ƵĽǶȻ� PID �޷�����
        roll_balance_cascade.angle_cycle.i_value_max        = 1000;      // �ǶȻ���������
        roll_balance_cascade.angle_cycle.i_value_pro        = 2.0f;       // �ǶȻ����ֱ���
        roll_balance_cascade.angle_cycle.out_max            = 50;      // �ǶȻ��������

        roll_balance_cascade.turn_cycle.i_value_max        = 1000;      // ת�򻷻�������
        roll_balance_cascade.turn_cycle.i_value_pro         = 2.0f;     // ת�򻷻��ֱ���
        roll_balance_cascade.turn_cycle.out_max            = 180;      //  ת���������

        // ��ʼ������ƽ����Ƶ��ٶȻ� PID �޷�����
        roll_balance_cascade.speed_cycle.i_value_max        = 500;      // �ٶȻ���������
        roll_balance_cascade.speed_cycle.i_value_pro         = 0.001f;   // �ٶȻ����ֱ�������0.001�죬���������٣�
        roll_balance_cascade.speed_cycle.out_max            = 9000;      // �ٶȻ��������

        // ����ƽ����Ƹ��� PID �� P/I/D ϵ��
        roll_balance_cascade.angle_cycle.p    = 10.0f;      // �ǶȻ� P
        roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
        roll_balance_cascade.angle_cycle.d    = 0.0f;       // �ǶȻ� D

        roll_balance_cascade.turn_cycle.p     = 0.2f;      // ת�� D
        roll_balance_cascade.turn_cycle.i     = 0.0f;      // ת�� D
        roll_balance_cascade.turn_cycle.d     = 0.0f;      // ת�� D

        roll_balance_cascade.speed_cycle.p    = 20.0f;      // �ٶȻ� P �� ����������ʱ����
        roll_balance_cascade.speed_cycle.i    = 3.0f;      // �ٶȻ� I
        roll_balance_cascade.speed_cycle.d    = 0.0f;      // �ٶȻ� D

    }
    else if(Mode_change_flag==3)
    {
               roll_balance_cascade.angular_speed_cycle.i_value_max     = 1000;      // ���ٶȻ���������
               roll_balance_cascade.angular_speed_cycle.i_value_pro    = 0.1f;       // ���ٶȻ����ֱ���
               roll_balance_cascade.angular_speed_cycle.out_max        = 50;      // ���ٶȻ��������

               // ��ʼ������ƽ����ƵĽǶȻ� PID �޷�����
               roll_balance_cascade.angle_cycle.i_value_max        = 1000;      // �ǶȻ���������
               roll_balance_cascade.angle_cycle.i_value_pro         = 2.0f;       // �ǶȻ����ֱ���
               roll_balance_cascade.angle_cycle.out_max            = 10000;      // �ǶȻ��������------------��������������,�н��ٶ�,���ԽǶȻ�ֻ���м����ò���Ϊֱ�����

               roll_balance_cascade.turn_cycle.i_value_max        = 1000;      // ת�򻷻�������
               roll_balance_cascade.turn_cycle.i_value_pro         = 2.0f;     // ת�򻷻��ֱ���
               roll_balance_cascade.turn_cycle.out_max            = 180;      //  ת���������

               // ��ʼ������ƽ����Ƶ��ٶȻ� PID �޷�����
               roll_balance_cascade.speed_cycle.i_value_max        = 500;      // �ٶȻ���������
               roll_balance_cascade.speed_cycle.i_value_pro         = 0.005f;   // �ٶȻ����ֱ���
               roll_balance_cascade.speed_cycle.out_max            = 9000;      // �ٶȻ��������

               // ����ƽ����Ƹ��� PID �� P/I/D ϵ��
               roll_balance_cascade.angular_speed_cycle.p    = 0.008f;      // ���ٶȻ� P
               roll_balance_cascade.angular_speed_cycle.i    = 0.0f;      // ���ٶȻ� I
               roll_balance_cascade.angular_speed_cycle.d    = 0.0f;      // ���ٶȻ� D

               roll_balance_cascade.angle_cycle.p    = 150.0f;      // �ǶȻ� P
               roll_balance_cascade.angle_cycle.i    = 0.0f;        // �ǶȻ� I
               roll_balance_cascade.angle_cycle.d    = 0.0f;        // �ǶȻ� D

               roll_balance_cascade.turn_cycle.p     = 0.2f;        // ת�� D
               roll_balance_cascade.turn_cycle.i     = 0.0f;        // ת�� D
               roll_balance_cascade.turn_cycle.d     = 0.0f;        // ת�� D

               roll_balance_cascade.speed_cycle.p    = 10.0f;       // �ٶȻ� P
               roll_balance_cascade.speed_cycle.i    = 3.0f;        // �ٶȻ� I
               roll_balance_cascade.speed_cycle.d    = 0.0f;        // �ٶȻ� D
    }


}

void Imu_init(void)
{
    if(Imu_type==1)
    {
        imu660ra_init();
    }
    else if(Imu_type==2)
    {
        imu660rb_init();
    }
    else if(Imu_type==3)
    {
        imu963ra_init();
    }

}

void Imu_attitude_scan(void)
{
    if(Imu_type==1)
    {
        imu660ra_get_gyro();                             // ��ȡ ����������
        imu660ra_get_acc();                              // ��ȡ ���ٶȼ�����
        quaternion_module_calculate(&roll_balance_cascade); // ������Ԫ����������̬����
    }
    else if(Imu_type==2)
    {
        imu660rb_get_gyro();                             // ��ȡ ����������
        imu660rb_get_acc();                              // ��ȡ ���ٶȼ�����
        quaternion_module_calculate(&roll_balance_cascade); // ������Ԫ����������̬����
    }
    else if(Imu_type==3)
    {
        imu963ra_get_gyro();                             // ��ȡ ����������
        imu963ra_get_acc();                              // ��ȡ ���ٶȼ�����
        quaternion_module_calculate(&roll_balance_cascade); // ������Ԫ����������̬����
    }

}

void Imu_lowpass_filter(void)
{
    static float last_gyro_y = 0.0f;

    if(Imu_type==1)
    {
        imu660ra_gyro_x=LowPassFilter(imu660ra_gyro_x, last_gyro_y, 0.1f);
        last_gyro_y=imu660ra_gyro_x;
    }
    else if(Imu_type==2)
    {
        imu660rb_gyro_y=LowPassFilter(imu660rb_gyro_y, last_gyro_y, 0.1f);
        last_gyro_y=imu660rb_gyro_y;
    }
    else if(Imu_type==3)
    {
        imu963ra_gyro_y=LowPassFilter(imu963ra_gyro_y, last_gyro_y, 0.1f);
        last_gyro_y=imu963ra_gyro_y;
    }
}



void IMU_text(void)
{

    ips_show_string(8*0, 16*0, "Pitch:");    ips_show_float(8*10,16*0, roll_balance_cascade.posture_value.pit,3,6);
    ips_show_string(8*0, 16*1, "ROLL:");     ips_show_float(8*10,16*1, roll_balance_cascade.posture_value.rol,3,6);
    ips_show_string(8*0, 16*2, "Yaw:");      ips_show_float(8*10,16*2, roll_balance_cascade.posture_value.yaw,3,6);

//       printf("%d,%d,%d\n",imu963ra_gyro_x, imu963ra_gyro_y, imu963ra_gyro_z);
//       printf("%d,%d,%d\n",imu660ra_gyro_x, imu660ra_gyro_y, imu660ra_gyro_z);
         printf("%d,%d,%d\n",imu660rb_gyro_x, imu660rb_gyro_y, imu660rb_gyro_z);

//       printf("%f,%f,%f\r\n",roll_balance_cascade.posture_value.pit, roll_balance_cascade.posture_value.rol, roll_balance_cascade.posture_value.yaw);
             system_delay_ms(10);
}
