/*
 * Agorithm.c
 *
 *  Created on: 2026��4��28��
 *      Author: misaka19754
 */
#include "zf_common_headfile.h"


/**
 * @brief  һ�׵�ͨ�˲���������ʽ����������
 * @param  current  ��ǰ����ֵ
 * @param  last     ��һ���˲����ֵ
 * @param  alpha    �˲�ϵ�� [0, 1]��ԽСԽƽ����Խ����ӦԽ��
 * @return          �����˲����ֵ
 * @note   �״�ʹ��ʱ��last Ӧ�� current ��ͬ����ɳ�ʼ��
 *         ʾ����filtered = LowPassFilter(raw, filtered, 0.2f);
 */
float LowPassFilter(float current, float last, float alpha)
{
    return last + alpha * (current - last);
}

/**
 * @brief  Ŀ��ֵ�����ƽ�������������ֱ��Ƕ����ã�
 * @param  target     ����Ŀ��ֵ
 * @param  step_size  ÿ�β���������ֵ��
 * @return            ���β������ֵ���ڲ��� static ����״̬��
 */
float StepApproach(float target, float step_size)
{
    static float current = 0.0f;

    if (current < target)
    {
        current += step_size;
        if (current > target)
            {
            current = target;
            }
    } else if (current > target)
    {
        current -= step_size;
        if (current < target)
            {
            current = target;
            }
    }
    return current;
}



/**
 * @brief  ���Ƕ�����һ���� [-180, 180] ��Χ��
 * @param  error  ԭʼ�Ƕ����ȣ�
 * @return        ��һ����ĽǶ����
 * @note   ���ڽ�� yaw ���� ��180�� �߽����䵼�� PID ��ը������
 *         ʾ����angle_error = AngleErrorNormalize(target_yaw - current_yaw);
 */
float AngleErrorNormalize(float error)
{
    if (error > 180.0f)
        return error - 360.0f;
    else if (error < -180.0f)
        return error + 360.0f;

    else
        return error;
}

/**
 * @brief  �ǶȲ����ƽ���֧�����·����ǿ�Ʒ����Զ�������180��߽磩
 * @param  target       ����Ŀ��Ƕȣ��ȣ�
 * @param  real_angle   ��ǰʵ�ʽǶȣ��ȣ����ڳ�ʼ����
 * @param  step_size    ÿ�β������仯�����ȣ�>0��
 * @param  mode         0=�Զ����·����1=ǿ��˳ʱ�루��·������-1=ǿ����ʱ�루��·����
 * @return              �������Ŀ��Ƕȣ��ȣ���һ���� [-180,180]��
 * @note   �״ε���ʱ���� real_angle ��ʼ���ڲ�״̬
 */
float StepApproachAngleMode(float target, float real_angle, float step_size, int mode)
{
    static float current = 0.0f;
    static uint8_t init = 0;

    if (!init) {
        current = real_angle;
        init = 1;
    }

    float err;  // ��ǰ���ֵ��Ŀ��ֵ���������ţ�����ֵ��ʾ��ת���ĽǶȣ�

    if (mode == 0) {
        // ���·����ȡ��С�ǶȲ��Χ [-180,180]
        err = target - current;
        if (err > 180.0f) err -= 360.0f;
        else if (err < -180.0f) err += 360.0f;
    }
    else if (mode == 1) {
        // ǿ��˳ʱ�룺��� = -˳ʱ����ת�ĽǶȣ�0~360��
        float cw = fmodf(current - target, 360.0f);
        if (cw < 0) cw += 360.0f;
        err = -cw;   // ˳ʱ��Ϊ��
    }
    else { // mode == -1
        // ǿ����ʱ�룺��� = +��ʱ����ת�ĽǶȣ�0~360��
        float ccw = fmodf(target - current, 360.0f);
        if (ccw < 0) ccw += 360.0f;
        err = ccw;   // ��ʱ��Ϊ��
    }

    // ��������
    if (fabsf(err) <= step_size) {
        current = target;
    } else {
        current += (err > 0 ? step_size : -step_size);
    }

    // ��һ������� [-180, 180]
    if (current > 180.0f) current -= 360.0f;
    if (current < -180.0f) current += 360.0f;

    return current;
}
