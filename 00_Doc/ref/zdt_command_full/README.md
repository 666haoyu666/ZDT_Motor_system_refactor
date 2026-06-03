# ZDT 完整协议命令库（参考归档，不参与编译）

- **来源**：原始可用版本 `ZDT-UART/command.{c,h}`，约 60 条命令构帧 + 多机 builder。
- **现状**：重构后只在 `04_Bsp/zdt_motor/zdt_cmd.{c,h}` 保留速度控制实际用到的 ~6 条
  （enable / speed / report + 多机拼帧 + 位置解析）。
- **用途**：需要位置模式、回零、PID、驱动配置等命令时，从这里把对应 `ZDT_Build*` builder
  捞回 `zdt_cmd`，按新命名/状态码（`zdt_status_t`, `OK==0`）改写即可。
- 本目录文件**不要加入 MDK/CubeMX 工程**。
