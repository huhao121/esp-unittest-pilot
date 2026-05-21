# CLAUDE.md

ESP32 单元测试练习工程：把纯逻辑抽成组件，在电脑上跑 host 单测。

## 开发环境

- ESP-IDF 版本：5.5.2，装在 `~/esp/esp-idf`。
- 任何 `idf.py` 命令前先 source 环境：

  ```bash
  source ~/esp/esp-idf/export.sh
  ```

- 两个 target：
  - **linux** —— 在电脑上跑单元测试，设置时要带 `--preview`：

    ```bash
    idf.py --preview set-target linux
    ```

  - **esp32s3** —— 烧到板子上跑：

    ```bash
    idf.py set-target esp32s3
    ```

## 硬件

- 主板：ESP32-S3 最小系统板。
- WS2812 RGB 灯：接 GPIO48。
- 按键：板载 BOOT 键，GPIO0。

## 工程约定

- **纯逻辑** 抽成组件，放 `components/<名字>/`。每个组件自带 `test_apps/` 测试工程，用 linux target 在电脑上 host 测。
- **碰硬件的代码**（GPIO、WS2812 驱动、按键读取等）单独放 `demo/`，只在板子上跑，**不进单测**。
- 测试与硬件代码之间通过组件接口解耦：逻辑层不直接调 ESP-IDF 外设 API，硬件层只做薄薄的胶水。

## 典型命令

在电脑上跑某个组件的单测：

```bash
source ~/esp/esp-idf/export.sh
cd components/<名字>/test_apps
idf.py --preview set-target linux
idf.py build
./build/<工程名>.elf       # 直接跑 elf，比 idf.py monitor 干脆
```

烧 demo 到板子：

```bash
source ~/esp/esp-idf/export.sh
cd demo
idf.py set-target esp32s3
idf.py build flash monitor
```

## 组件测试工程的坑

搭 `components/<名字>/test_apps/` 时踩过、以后建新组件直接照抄：

- **test_apps 找不到自己的组件**：顶层 `test_apps/CMakeLists.txt` 里要
  `set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../..")` 指到 `components/`，
  否则 build 时报找不到 lamp 之类的组件。
- **main 的 TEST_CASE 被链接器丢掉**：`main/CMakeLists.txt` 的
  `idf_component_register(... REQUIRES unity <组件> WHOLE_ARCHIVE)` 必须带 `WHOLE_ARCHIVE`。
- **linux 下 elf 跑完不退出**：FreeRTOS-on-linux 在 `app_main` 返回后不结束进程，
  直接跑会像“假死”。`app_main` 里跑完后主动退出：

  ```c
  unity_run_all_tests();
  exit(Unity.TestFailures == 0 ? 0 : 1);   // 给自动化一个明确退出码
  ```

## 看覆盖率（host 上 gcov/gcovr）

只给被测组件加 `--coverage`，用 CMake 开关关起来，平时构建/烧板子零影响。
以 lamp 为例（其它组件照搬）：

- 组件 `CMakeLists.txt` 里加开关，只插桩自己：

  ```cmake
  if(LAMP_COVERAGE)
      target_compile_options(${COMPONENT_LIB} PRIVATE --coverage -O0 -g)
      target_link_options(${COMPONENT_LIB} PUBLIC --coverage)
  endif()
  ```

- 一条命令跑全套（见 `components/lamp/test_apps/coverage.sh`）：

  ```bash
  source ~/esp/esp-idf/export.sh
  cd components/lamp/test_apps
  ./coverage.sh
  ```

- 手动等价流程：`idf.py -DLAMP_COVERAGE=ON build`（插桩，出 `.gcno`）→
  `./build/lamp_test.elf`（出 `.gcda`）→
  `gcovr --root ../.. --filter '.*/lamp\.c$' --branches build`（行+分支报告）。

注意：

- `--filter '.*/<源文件>\.c$'` 不能省，否则 gcovr 会把一堆 ESP-IDF 文件也列进来。
- 报告产物都在 `build/` 下，已被 `.gitignore` 忽略。
- 改代码重测前旧 `.gcda` 会累积命中；想清零就 `idf.py fullclean`（`coverage.sh` 用全新 build 规避）。
