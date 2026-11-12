# DJI M3508 棘轮(detent)手感示例

用位置环（`motor-controller,pv` 外环 + `motor-controller,vo` 内环）模拟旋钮/棘轮的离散档位手感。

## 行为

- 输出轴一圈等分为 `DETENT_COUNT` 档（默认 12 档，每档 30°）。
- 控制循环（5 ms）读取 `sum_angle`，目标始终为最近的档位，通过 `motor_set_angle()` 下发。
- 拨动轴时位置环产生回弹力；越过两档中点（加 `DETENT_HYSTERESIS_DEG` 滞回）后目标跳到下一档，产生“咔哒”换档感。
- 启动时等待首帧反馈，执行 `SET_ZERO`，并以当前位置所在档位为起点。

## 参数

| 位置 | 参数 | 作用 |
| --- | --- | --- |
| `src/main.c` | `DETENT_COUNT` | 档位数量 |
| `src/main.c` | `DETENT_HYSTERESIS_DEG` | 换档滞回，防止中点抖动 |
| `app.overlay` | 外环 `out_max` (rpm) | 限制回弹速度，越小越软 |
| `app.overlay` | 内环 `out_max` | 限制回弹力度 |
| `app.overlay` | `pos_k_p` / `pos_k_d` | 档位“刚度”与阻尼 |

PID 增益是起始值，需要在实际硬件上整定。

## 设备树

`m3508_motor` 的 `id` 必须与电机电调指示灯的闪烁次数一致。`rx_id` 为 `0x200 + id`；`id` 为 1-4 时 `tx_id` 为 `0x200`，5-8 时为 `0x1FF`。

## 构建

```shell
west build -b robomaster_board_c samples/motor/dji_m3508_detent --pristine
```

本示例尚未在硬件上验证。
