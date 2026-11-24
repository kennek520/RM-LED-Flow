# 电控组第二次培训作业

- **李青霖**
- ***20263005044***

## 一、实现思路

三个 LED 接在 PA0、PA1、PA2 上。CubeMX 里把这三个引脚配置成推挽输出，再用 User Label 分别命名为 `LED0`、`LED1`、`LED2`。这样一来 CubeMX 会在 `main.h` 里自动生成 `LED0_Pin`、`LED0_GPIO_Port` 这样的宏，代码里直接调用宏就行，不用记具体是第几号引脚，以后改接线也只需要在 CubeMX 里改。

三个灯要做的事只有一件：点亮其中一个，另外两个灭掉。所以我把它整理成两个数组，用下标 0/1/2 访问：

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

于是"流水灯"就变成了"让下标每 500ms 加一，加到 3 回到 0"。基础版本直接在循环里调用 `LED_Show(i)` 再 `HAL_Delay(500)`，现象是对的。

## 二、加分项：摆脱 HAL_Delay 死等

### 1. HAL_Delay 的问题

`HAL_Delay(500)` 内部是不停读系统 tick 的忙等，这 500ms 里 CPU 一直卡在 while 循环里，什么都做不了。三个灯轮流各等 500ms，等于主循环几乎所有时间都在空转。

### 2. 改成"查时间"而不是"等时间"

主循环不再等待，而是每一圈读一次当前时间，和"上次换灯的时刻"作比较：差值达到 500ms 才换灯，没到就立刻继续往下走。

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

这样换灯的间隔仍然是 500ms，但主循环一秒能转几十万圈，中间的时间可以拿去做按键扫描、串口接收之类的事情，不会被等待卡住。

### 3. 一个细节：为什么用减法比较时间

`HAL_GetTick()` 返回的是 `uint32_t`，大约 49.7 天会溢出归零。所以判断写成 `now - s_last_tick >= 500`，而不是 `now >= s_last_tick + 500`：

- 减法比较是无符号运算，即使 `now` 已经溢出归零、`s_last_tick` 还很大，差值依然等于真实的时间差；
- 加法比较一旦 `s_last_tick + 500` 溢出，判断就会失效，可能卡住或者立刻误触发。

### 4. 怎么说明它真的没有被阻塞

主循环里放了一个计数器：

```c
volatile uint32_t g_loop_count = 0;
```

用调试器暂停程序后观察这个变量：

| 版本 | 一秒增长 |
|---|---|
| `HAL_Delay` 版 | 2 |
| 非阻塞版 | 几十万 |

同样是 500ms 换一次灯，CPU 的空闲程度差了几个数量级。

### 5. 另一种实现：定时器中断

除了上面的轮询方式，也可以用 TIM6 每 500ms 产生一次中断，在中断回调里换灯，主循环里完全不写换灯代码。TIM6 参数：Prescaler = 7199、Counter Period = 4999，72MHz / 7200 / 5000 = 2Hz，正好 500ms。

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

中断里只做写寄存器这种微秒级的操作，不放延时和打印。
