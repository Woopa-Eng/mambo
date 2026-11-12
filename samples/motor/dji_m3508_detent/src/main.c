/*
 * DJI M3508 棘轮(detent)手感示例
 *
 * 把输出轴 360 度均分为 DETENT_COUNT 个档位。位置环始终把目标设在离当前
 * 位置最近的档位上：拨动轴时位置环产生回弹力，越过两档中点后目标跳到下一档，
 * 形成“咔哒”一下的换档手感。
 *
 * 位置使用 sum_angle（输出轴多圈累计角度，单位度），目标与其处于同一坐标系，
 * 因此跨圈时无需处理角度回绕。
 *
 * Copyright (c) 2024 ttwards
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/motor.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(dji_m3508_detent, LOG_LEVEL_INF);

#define M3508_NODE DT_NODELABEL(m3508_motor)
static const struct device *m3508_motor = DEVICE_DT_GET(M3508_NODE);

/* 档位数量，输出轴一圈内均分 */
#define DETENT_COUNT 12
#define DETENT_STEP  (360.0f / DETENT_COUNT)
/* 滞回: 越过中点再多走该角度才换档，防止在中点附近来回抖动 */
#define DETENT_HYSTERESIS_DEG 1.5f
#define CONTROL_PERIOD_MS     5
#define LOG_PERIOD_MS         500

int main(void)
{
	motor_status_t status;
	int detent = 0;
	int64_t last_log = 0;

	if (!device_is_ready(m3508_motor)) {
		LOG_ERR("M3508电机设备未就绪");
		return -ENODEV;
	}

	motor_control(m3508_motor, ENABLE_MOTOR);

	/* 等待首帧反馈，以上电位置所在档位作为起点，避免上电瞬间猛拉 */
	while (motor_get(m3508_motor, &status) != 0 || !status.online) {
		k_msleep(10);
	}
	motor_control(m3508_motor, SET_ZERO);
	k_msleep(500);
	if (motor_get(m3508_motor, &status) == 0) {
		detent = (int)roundf(status.sum_angle / DETENT_STEP);
	}
	LOG_INF("棘轮示例: %d 档, 每档 %.2f°", DETENT_COUNT, (double)DETENT_STEP);

	while (1) {
		if (motor_get(m3508_motor, &status) == 0 && status.online) {
			/* 当前位置相对已选档位的偏差(度) */
			float err = status.sum_angle - detent * DETENT_STEP;

			if (err > DETENT_STEP / 2 + DETENT_HYSTERESIS_DEG) {
				detent++;
				LOG_INF("档位 -> %d (%.1f°)", detent,
					(double)(detent * DETENT_STEP));
			} else if (err < -(DETENT_STEP / 2 + DETENT_HYSTERESIS_DEG)) {
				detent--;
				LOG_INF("档位 -> %d (%.1f°)", detent,
					(double)(detent * DETENT_STEP));
			}

			motor_set_angle(m3508_motor, detent * DETENT_STEP);

			if (k_uptime_get() - last_log >= LOG_PERIOD_MS) {
				last_log = k_uptime_get();
				LOG_INF("档位 %d 目标 %.1f° 实际 %.1f° 速度 %.1f rpm", detent,
					(double)(detent * DETENT_STEP), (double)status.sum_angle,
					(double)status.rpm);
			}
		}
		k_msleep(CONTROL_PERIOD_MS);
	}

	return 0;
}
