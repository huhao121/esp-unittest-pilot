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
idf.py monitor          # 或直接运行 build/*.elf 看测试输出
```

烧 demo 到板子：

```bash
source ~/esp/esp-idf/export.sh
cd demo
idf.py set-target esp32s3
idf.py build flash monitor
```
