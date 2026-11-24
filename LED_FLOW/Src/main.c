/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 作业2 · STM32F103C8T6 三灯流水灯（最终版，三模式可切换）
  ******************************************************************************
  *  工程：D:\STM32\LED_FLOW
  *  引脚：LED0 = PA0，LED1 = PA1，LED2 = PA2（高电平点亮）
  *
  *  ┌───────────────────────────────────────────────────────────────────────┐
  *  │  只需要改下面这一行，就能在三种实现之间切换：                          │
  *  │                                                                       │
  *  │    #define CODE_VERSION   0   基础任务：HAL_Delay 阻塞版              │
  *  │    #define CODE_VERSION   1   加分项A：HAL_GetTick 非阻塞（默认）     │
  *  │    #define CODE_VERSION   2   加分项B：TIM6 定时器中断（需先配 TIM6）  │
  *  │                                                                       │
  *  │  三种版本的“现象”完全一样：三灯每 500ms 依次亮一盏，循环。             │
  *  │  区别只在“这 500ms 是怎么数出来的”，以及 CPU 有没有被占用。           │
  *  └───────────────────────────────────────────────────────────────────────┘
  *
  *  切换方法：改 CODE_VERSION 的数字 → Ctrl+F9 重新编译 → 点 ▶ 重新烧录。
  *  代码其它部分一个字都不用动。
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ======================= 选择要编译的版本 ======================= */
#define CODE_VERSION        1

/*   0 = 基础任务（HAL_Delay 阻塞）
 *   1 = 加分项 A（HAL_GetTick 时间戳非阻塞）—— 默认，推荐提交这个
 *   2 = 加分项 B（TIM6 定时器中断）
 *       注意：选 2 之前必须先在 CubeMX 里把 TIM6 配好并 GENERATE CODE，
 *       否则会报 “htim6 undeclared”（这个工程目前还没配 TIM6）。
 */
/* =============================================================== */

/* 流水灯每步的间隔（毫秒）。作业要求 500 */
#define LED_STEP_MS         500u

/* 点亮/熄灭电平：三个灯都是外接（PA0 → 电阻 → LED正极 → LED负极 → GND），
 * 属于“高电平点亮”。如果哪个灯该亮时反而灭，说明它接反了，
 * 把这两个宏对调即可（或单独把那颗灯的线调过来）。 */
#define LED_ON_LEVEL        GPIO_PIN_SET
#define LED_OFF_LEVEL       GPIO_PIN_RESET
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 三个灯的引脚表。LED0_Pin / LED0_GPIO_Port 这些宏是 CubeMX 按 User Label
 * 自动生成在 main.h 里的，所以代码里不需要出现 PA0 这种具体脚号；
 * 以后换引脚只要在 CubeMX 里改，这段代码一行都不用动。 */
static GPIO_TypeDef * const LED_PORT[3] = { LED0_GPIO_Port, LED1_GPIO_Port, LED2_GPIO_Port };
static const uint16_t       LED_PIN [3] = { LED0_Pin,        LED1_Pin,        LED2_Pin        };

/* 主循环计数器：只用来看“CPU 到底有没有被卡住”。
 * 用调试器暂停程序时观察这个值：
 *   版本 0（阻塞）   → 每 500ms 才 +1，一秒只涨 2
 *   版本 1/2（非阻塞）→ 一秒涨几十万
 * 加 volatile 是防止编译器把它优化掉。 */
volatile uint32_t g_loop_count = 0;

#if (CODE_VERSION == 1)
/* 非阻塞流水灯的状态变量 */
static uint32_t s_last_tick = 0;    /* 上一次换灯的时刻（HAL_GetTick 的毫秒数） */
static uint8_t  s_led_index = 0;    /* 当前亮的是第几个灯，0/1/2 */
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void LED_Show(uint8_t idx);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  只点亮三个灯中的第 idx 个，其余全部熄灭
  * @param  idx 0 / 1 / 2，分别对应 LED0(PA0) / LED1(PA1) / LED2(PA2)
  */
static void LED_Show(uint8_t idx)
{
  for (uint8_t i = 0; i < 3u; i++)
  {
    /* 是第 idx 个就写“点亮电平”，其它写“熄灭电平” */
    HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i],
                      (i == idx) ? LED_ON_LEVEL : LED_OFF_LEVEL);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
#if (CODE_VERSION == 1)
  s_last_tick = HAL_GetTick();       /* 记下起始时刻 */
  s_led_index = 0u;
#endif

#if (CODE_VERSION == 2)
  HAL_TIM_Base_Start_IT(&htim6);     /* 启动 TIM6 并打开更新中断 */
#endif

  LED_Show(0u);                      /* 上电先点亮 LED0（同时把另两个灭掉） */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if (CODE_VERSION == 0)
    /* ========================= 版本 0：基础任务 =========================
     * HAL_Delay 阻塞版：亮一盏 → 死等 500ms → 换下一盏。
     * 这 500ms 里 CPU 完全被占用，什么也做不了，所以叫“死等”。
     * ================================================================== */
    for (uint8_t i = 0; i < 3u; i++)
    {
      LED_Show(i);
      HAL_Delay(LED_STEP_MS);
      g_loop_count++;
    }

#elif (CODE_VERSION == 1)
    /* ====================== 版本 1：加分项 A（默认）======================
     * 非阻塞版：主循环全速转圈，每圈只问一句“到点了吗”。
     *   到点   → 换下一盏灯，并更新“上次换灯时刻”
     *   没到点 → 什么也不做，立刻往下走
     * CPU 永远不会停在等待里，这是“状态机”思路的入门形态。
     * ================================================================== */
    uint32_t now = HAL_GetTick();            /* 读一次系统毫秒计数 */
    g_loop_count++;                          /* 主循环跑了多少圈 */

    if (now - s_last_tick >= LED_STEP_MS)    /* 用减法比较，自动处理 32 位溢出 */
    {
      s_last_tick = now;                     /* 记下这次换灯的时刻 */
      s_led_index = (uint8_t)((s_led_index + 1u) % 3u);
      LED_Show(s_led_index);                 /* 换下一盏灯 */
    }

    /* ↓↓↓ 这一行以下就是“非阻塞”的价值所在：
     * 主循环每圈都会经过这里，你随时可以往下加按键扫描、串口收命令、
     * 传感器采样、蜂鸣器节奏……它们都不会被 500ms 的等待拖慢。
     * 版本 0 做不到：它一卡就是 500ms，按键要按住了才响应。 */

#else
    /* ====================== 版本 2：加分项 B ============================
     * TIM6 定时器中断版：换灯由硬件定时器在中断里完成，
     * 主循环里一行换灯代码都没有，CPU 完全不用管计时。
     * 前提：CubeMX 里 TIM6 已配好（PSC=7199, ARR=4999, 开 TIM6 中断）。
     * ================================================================== */
    g_loop_count++;                          /* 主循环只做计数，换灯在中断里 */
#endif

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LED0_Pin LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* CubeMX 默认把初始电平配成 High（三个脚上电瞬间都是高电平），
   * 高电平点亮时三个灯会同时闪一下，这里补一句先全灭。 */
  HAL_GPIO_WritePin(GPIOA, LED0_Pin|LED1_Pin|LED2_Pin, LED_OFF_LEVEL);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
#if (CODE_VERSION == 2)
/* ============ 版本 2 专用：定时器更新中断回调 ============
 * 每 500ms 被硬件自动调用一次，在这里换灯。
 * TIM6 配置：PSC = 7199 → 72MHz/7200 = 10kHz
 *            ARR = 4999 → 10kHz/5000  = 2Hz → 正好 500ms
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t idx = 0;

  if (htim->Instance == TIM6)          /* 确认是 TIM6 触发的（回调可能被多个定时器共用） */
  {
    idx = (uint8_t)((idx + 1u) % 3u);
    LED_Show(idx);                     /* 中断里只做“写寄存器”这种微秒级的短事 */
  }
}
#endif
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
