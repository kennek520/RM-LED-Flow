# 电控组第二次培训作业

- **李青霖**
- ***20263005044***

## 一、实现思路

硬件上三个 LED 分别接在 PA0、PA1 和 PA2。在 CubeMX 中将这三个引脚配置为推挽输出，并设置 User Label 为 `LED0`、`LED1`、`LED2`。生成代码后，`main.h` 中会自动生成 `LED0_Pin`、`LED0_GPIO_Port` 等宏定义，应用层直接调用即可，无需硬编码具体引脚号；后续若硬件接线变更，只需在 CubeMX 中重新分配引脚，业务代码基本无需修改。

流水灯的核心逻辑是每次仅点亮其中一盏，其余两盏熄灭。为了便于按顺序轮询控制，将端口和引脚分别封装进两个数组，通过下标 `0`、`1`、`2` 进行索引：

```c
static GPIO_TypeDef * const LED_PORT[3] = { LED0_GPIO_Port, LED1_GPIO_Port, LED2_GPIO_Port };
static const uint16_t       LED_PIN [3] = { LED0_Pin,        LED1_Pin,        LED2_Pin        };

/* 只点亮第 idx 个灯，其余熄灭 */
static void LED_Show(uint8_t idx)
{
  for (uint8_t i = 0; i < 3u; i++)
  {
    HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i],
                      (i == idx) ? LED_ON_LEVEL : LED_OFF_LEVEL);
  }
}
```

封装该函数后，流水灯即转化为每 500ms 使下标自增 1、满 3 归零的周期操作。常规做法是在 `while(1)` 中交替调用 `LED_Show(i)` 与 `HAL_Delay(500)`，虽然功能正常，但在实际工程中存在明显局限。

## 二、加分项：摆脱 HAL_Delay 死等

`HAL_Delay(500)` 的底层是通过死循环不断查询系统 tick 实现阻塞延时，在这 500ms 内 CPU 完全被占用，无法响应其他任务；三个灯轮流延时一轮，主循环绝大部分时间都处于空转状态。要释放 CPU 资源，需要将阻塞延时替换为非阻塞的时间戳轮询——主循环不作停留，每次执行时读取当前系统时间，并与上次换灯的时刻作差：未达到 500ms 则直接跳过，继续执行后续逻辑；达到设定间隔后再切换状态并更新时间戳：

```c
static uint32_t s_last_tick = 0;   /* 上次换灯的时刻 */
static uint8_t  s_led_index = 0;   /* 当前亮的是第几个灯 */

while (1)
{
  uint32_t now = HAL_GetTick();          /* 读一次系统毫秒计数 */

  if (now - s_last_tick >= LED_STEP_MS)  /* 到点了吗 */
  {
    s_last_tick = now;                   /* 记下这次换灯的时刻 */
    s_led_index = (uint8_t)((s_led_index + 1u) % 3u);
    LED_Show(s_led_index);               /* 换下一盏灯 */
  }
}
```

修改后，视觉上依然是 500ms 切换一次状态，但 `while(1)` 每秒已能执行数十万次循环，空闲出的 CPU 时间可用于处理按键扫描、串口通信等并发任务。

同时在时间比较的实现上还需要注意到数据溢出问题，即 `HAL_GetTick()` 的返回值为 `uint32_t` 类型，系统连续运行约 49.7 天后会溢出归零。因此超时判断必须采用减法形式 `now - s_last_tick >= 500`，而不能写成 `now >= s_last_tick + 500`。无符号整数的减法运算在底层会自动处理回绕，即便 `now` 刚溢出归零而 `s_last_tick` 仍接近最大值，两者的差值依然等于真实经过的毫秒数；若采用加法，`s_last_tick + 500` 一旦溢出便会变为一个极小的数值，导致条件误触发，破坏时序逻辑。

为了量化验证主循环的阻塞情况，可以在 `while(1)` 中加入一个全局计数变量，每次循环自增 1：

```c
volatile uint32_t g_loop_count = 0;
```

连接调试器运行一秒后暂停，观察监控窗口中的变量值即可看出差异：使用 `HAL_Delay` 时，一秒内该变量仅能累加到 `2`；改用时间戳轮询后，一秒内可增长至 **几十万**，在保持相同亮灯周期的前提下，主循环的吞吐能力提升了数个数量级。

除了在主循环中轮询，也可以将定时任务交由硬件定时器处理。以基本定时器 TIM6 为例，在 72MHz 时钟源下，将预分频系数（Prescaler）设为 `7199`，自动重装载值（Counter Period）设为 `4999`，计算得出更新中断频率为 `72MHz / 7200 / 5000 = 2Hz`，即每 500ms 触发一次中断：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t idx = 0;

  if (htim->Instance == TIM6)
  {
    idx = (uint8_t)((idx + 1u) % 3u);
    LED_Show(idx);
  }
}
```

采用中断方式后，`main` 函数的 `while(1)` 中无需再包含任何 LED 控制代码。编写中断回调函数时需遵循基本规范：内部仅执行置标志位、操作寄存器等微秒级指令，严禁调用延时函数或串口打印等耗时操作，以免影响系统的整体实时性。
