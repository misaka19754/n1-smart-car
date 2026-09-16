/*
 * Agorithm.h
 *
 *  Created on: 2026��4��28��
 *      Author: misaka19754
 */

#ifndef CODE_AGORITHM_H_
#define CODE_AGORITHM_H_

float LowPassFilter(float current, float last, float alpha);

float StepApproach(float target, float step_size);

float AngleErrorNormalize(float error);

float StepApproachAngleMode(float target, float real_angle, float step_size, int mode);

#endif /* CODE_AGORITHM_H_ */
