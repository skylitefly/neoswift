# VATSIM 私货审查报告

> 审查范围：`pilotclient` 代码库中所有与 VATSIM 强绑定的代码，包括硬编码 URL、专有依赖、协议实现及行为差异。

---

## 一、硬编码 VATSIM URL

**文件：** `resources/share/shared/bootstrap/bootstrap.json`

所有 VATSIM 服务地址均硬编码，无法在不修改配置的情况下用于第三方网络：

| 用途 | URL |
|------|-----|
| 网络数据 | `https://data.vatsim.net/v3/vatsim-data.json` |
| 服务器列表 | `https://data.vatsim.net/v3/vatsim-servers.json` |
| FSD HTTP | `http://fsd.vatsim.net` |
| METAR | `https://metar.vatsim.net/metar.php` |
| 状态文件 | `https://status.vatsim.net/status.json` |
| 语音服务器（AFV）| `https://voice1.vatsim.net/` |
| AFV 地图 | `https://afv-map.vatsim.net/` |
| JWT 认证 | `https://auth.vatsim.net/api/fsd-jwt` |

---

## 二、专有认证库依赖（最严重）

**文件：** `cmake/modules/FindVATSIMAuth.cmake`、`CMakeLists.txt`（L82-88, L130-132）

- 依赖闭源专有库 `VATSIMAuth`（`libvatsimauth.so/.dylib/.dll`）
- 头文件：`vatsim/vatsimauth.h`
- 构建开关：`SWIFT_VATSIM_SUPPORT`（默认 **ON**）
- 编译时嵌入 VATSIM client ID 和 Key（通过 `cmake/tools.cmake` 的 `load_vatsim_key()` 从外部 JSON 载入）
- 该库为闭源二进制，第三方无法获取，是最大的单点阻塞

---

## 三、VATSIM 专有 FSD 协议

**文件：** `src/core/fsd/fsdclient.h`（L50-54）、`src/core/fsd/fsdclient.cpp`

### 3.1 协议版本

```cpp
constexpr int PROTOCOL_REVISION_CLASSIC         = 9;   // 标准 FSD
constexpr int PROTOCOL_REVISION_VATSIM_ATC      = 10;
constexpr int PROTOCOL_REVISION_VATSIM_AUTH     = 100;
constexpr int PROTOCOL_REVISION_VATSIM_VELOCITY = 101; // VATSIM 默认
```

**选择逻辑**（`fsdclient.cpp:148-150`）：

```cpp
const int protocolRev = (server.getServerType() == CServer::FSDServerVatsim)
    ? PROTOCOL_REVISION_VATSIM_VELOCITY  // 101
    : PROTOCOL_REVISION_CLASSIC;         // 9
```

### 3.2 登录握手流程对比

**VATSIM（协议 101）：**
```
TCP 连接建立
  → 服务器发 FsdIdentification（含 challenge）
    → handleFsdIdentification()  [#ifdef SWIFT_VATSIM_SUPPORT]
      → vatsim_auth_* 处理 challenge（调用专有库）
        → sendClientIdentification()
          → 向 auth.vatsim.net 请求 JWT token
            → sendLogin(token)  ← password 字段为 JWT token
```

**其他服务器（协议 9）：**
```
TCP 连接建立
  → handleSocketConnected()
    → sendLogin()  ← password 字段为明文密码，直接登录
```

### 3.3 专有消息类型（均在 `#ifdef SWIFT_VATSIM_SUPPORT` 内）

| 消息处理器 | 用途 |
|-----------|------|
| `handleFsdIdentification()` | 接收服务器发来的身份识别+challenge |
| `sendClientIdentification()` | 发送客户端身份+challenge 响应 |
| `handleAuthChallenge()` | 处理认证挑战 |
| `handleAuthResponse()` | 验证服务器响应（不匹配则强制断开） |

`handleAuthResponse` 中有硬性校验（`fsdclient.cpp:1135`）：
```
"The server you are connected to is not a VATSIM server. Disconnecting!"
```

### 3.4 SYS_UID 字段

`fsdclient.cpp:948-951`：非 VATSIM 时 `SYS_UID=` 字段发送为空字符串；VATSIM 时由专有库 `vatsim_get_system_unique_id()` 生成。

---

## 四、VATSIM 专用数据读取器

**目录：** `src/core/vatsim/`，通过 `src/core/webdataservices.h/cpp` 统一管理。

| 类名 | 数据源 | 用途 |
|------|--------|------|
| `CVatsimStatusFileReader` | `status.vatsim.net` | 引导启动，提供其他 URL |
| `CVatsimServerFileReader` | `data.vatsim.net/v3/vatsim-servers.json` | FSD 服务器列表（连接界面下拉框数据源）|
| `CVatsimDataFileReader` | `data.vatsim.net/v3/vatsim-data.json` | 在线用户数据，用于空域监控/雷达/地图 |
| `CVatsimMetarReader` | `metar.vatsim.net/metar.php` | METAR 天气，显示在天气面板 UI |

**关键问题：这四个读取器在应用启动时即初始化，与当前连接的是哪个服务器完全无关。** 即使连接的是 Skylite 服务器，它们仍会持续轮询 VATSIM 的基础设施。

### 数据流向

```
CVatsimDataFileReader
  ├── airspacemonitor.cpp  → 空域监控（更新雷达/地图上的其他飞机和管制员）
  ├── webdataservices.cpp  → getUsersForCallsign / getAtcStationsForCallsign
  └── webdataservices.cpp  → getFlightPlanRemarksForCallsign（读取他人飞行计划备注）

CVatsimMetarReader
  └── webdataservices.cpp  → getMetars / getMetarForAirport → 天气面板 UI
```

---

## 五、飞行员评级系统（PilotRating）

**文件：** `src/core/fsd/enums.h`（L31-40）

```cpp
enum class PilotRating { Unknown, Student(P1), VFR(P2), IFR(P3), Instructor, Supervisor }
```

### 用途一：FSD 协议层（每次登录和位置更新都携带）

- `fsdclient.cpp:300` — `AddPilot` 登录消息字段
- `fsdclient.cpp:352` — `PilotDataUpdate` 每次位置更新携带
- `contextnetworkimpl.cpp:320` — 连接时**硬编码**为 `PilotRating::Student`（P1）

### 用途二：飞行计划 UI

- `flightplancomponent.cpp:143,707` — 飞行计划对话框中的 `cb_PilotRating` 下拉框，选中值编码进飞行计划 Remarks 字段（VATSIM 特有的 `/PR` 格式）

ATC 评级图标同样为 VATSIM 专用：`src/misc/icons/vatsim/`（OBS/S1/S2/S3/C1/C3/I1/I3/SUP/MNT）

---

## 六、AFV 语音系统

AFV（Audio for VATSIM）是 VATSIM 开发的专有语音协议。

### 6.1 实现方式

**全部实现代码在 `src/core/afv/` 中，是自行实现的，不是闭源库依赖。** 包含：

- `connection/apiserverconnection.cpp` — REST API 客户端
- `connection/clientconnection.cpp` — UDP 音频连接
- `crypto/` — AEAD 加密（基于 msgpack）
- `audio/` — 音频输入输出处理
- `model/afvmapreader.cpp` — 定时轮询在线管制员收发器位置

### 6.2 绑定点

| 绑定项 | 位置 | 说明 |
|--------|------|------|
| 语音服务器 URL | `bootstrap.json` → `voice1.vatsim.net` | 通过 `globalsetup.h::getAfvApiServerUrl()` 读取 |
| AFV 地图 URL | `bootstrap.json` → `afv-map.vatsim.net` | `afvmapreader.cpp:36` 每 3 秒轮询 |
| REST 认证端点 | `/api/v1/auth`（用户名/密码换 JWT）| 需要服务端实现 AFV API |
| 日志分类 | `CLogCategories::vatsimSpecific()` | 4 个文件明确标记为 VATSIM 专用 |

### 6.3 AFV 的问题

AFV 协议本身已由 VATSIM 开源/公开文档，客户端侧实现可以移植。但 **VATSIM 未公开服务端实现**，第三方网络若要使用语音，需要自行实现兼容 AFV API 的后端。与数据读取器相同，AFV 客户端在应用启动时即连接 `voice1.vatsim.net`，不受当前 FSD 连接状态影响。

---

## 七、服务器设置 UI 中的 Eco / Type 下拉框

**文件：** `src/gui/editors/serverform.cpp`、`src/misc/network/server.cpp`、`src/misc/network/ecosystem.h`

### 7.1 可选值

**Eco（生态系统）：**

| 选项 | 枚举值 |
|------|--------|
| Unspecified | `CEcosystem::Unspecified` |
| NoSystem | `CEcosystem::NoSystem` |
| VATSIM | `CEcosystem::VATSIM` |
| SwiftTest | `CEcosystem::SwiftTest` |
| Swift | `CEcosystem::Swift`（未来保留）|
| PrivateFSD | `CEcosystem::PrivateFSD` |

**Type（服务器类型）：**

| 选项 | 枚举值 |
|------|--------|
| FSD Server (VATSIM) | `CServer::FSDServerVatsim` |
| FSD Server | `CServer::FSDServer` |
| Voice Server (VATSIM) | `CServer::VoiceServerVatsim` |
| Voice Server | `CServer::VoiceServer` |
| Web Service | `CServer::WebService` |
| Unspecified | `CServer::Unspecified` |

### 7.2 双向联动逻辑

`serverform.cpp:104-122`：两个下拉框有双向自动同步。

- 改 **Eco → VATSIM 或 SwiftTest** → Type 自动切换为 `FSDServerVatsim`（`server.cpp:93-94`）
- 改 **Type** → Eco 从该类型的 dummy server 反推并同步

联动只在结果非 `Unspecified` 时生效，两者**可被独立设置为不一致的组合**。

### 7.3 各自控制的行为

**Type** 控制：
- FSD 协议版本（`FSDServerVatsim` → 101，其他 → 9），这是**整个连接握手流程走哪条路的决定性因素**

**Eco** 控制：
- 空域范围限制（`VATSIM` → 125 NM，其他 → 无限制）（`airspacemonitor.cpp:1411-1413`）
- JWT 认证 vs 明文密码（`sendClientIdentification` 内的 ecosystem 判断）
- FSD 负载均衡是否启用（`fsdclient.cpp:1669`）

### 7.4 不同组合的实际效果

| Eco | Type | 效果 |
|-----|------|------|
| `PrivateFSD` | `FSDServer` | **正确配置**：协议 9，明文密码直接登录，无范围限制 |
| `PrivateFSD` | `FSDServerVatsim` | **连接挂死**：协议 101，等待服务器发 `FsdIdentification`，第三方服务器不会发，连接永久阻塞，无错误提示 |
| `VATSIM` | `FSDServer` | **行为混乱**：协议 9 直接登录（JWT 路径不触发），但空域被限 125 NM |
| `VATSIM` | `FSDServerVatsim` | 完整 VATSIM 流程（协议 101 + JWT + 范围限制）|

连接 Skylite 等第三方服务器的**正确配置为 Eco=`PrivateFSD`，Type=`FSDServer`**。若用户误选 `FSDServerVatsim`，连接将静默挂死，对用户完全不透明。

---

## 八、其他嵌入引用

- **法律文件** `resources/share/legal/`：`about.html`、`legal.html`、`privacy.html`、`3rdparty.html` 中包含 VATSIM 隐私政策、论坛链接
- **VATSIM 读取器设置 UI** `src/gui/components/settingsvatsimreaders*`：专用于配置 VATSIM 数据读取器轮询间隔的设置页面
- **FSD 设置** `src/misc/network/fsdsetup.h`：`VATSIMDefault` 标志位组合，`vatsimStandard()` 静态方法（在 Euroscope Tower view 硬编码使用）

---

## 九、总结

### 按修改代价分级

| 类别 | 能否绕过 | 修改代价 |
|------|---------|---------|
| 硬编码 URL（bootstrap.json）| 是，修改配置即可 | 低 |
| VATSIM 数据读取器无条件运行 | 需代码修改（加 ecosystem 判断）| 中 |
| 飞行员评级系统（硬编码 P1）| 需代码修改 | 低 |
| FSD 协议分支逻辑 | 现有逻辑已正确处理非 VATSIM | 无需修改 |
| AFV 语音绑定 VATSIM 基础设施 | 需第三方部署 AFV 后端 | 高 |
| 专有 `VATSIMAuth` 库 | 不可绕过（闭源二进制）| **不可移除** |

### 核心结论

即使将 bootstrap.json 中所有 URL 全部替换为第三方地址，编译后的客户端仍因以下原因**无法在 Skylite 等第三方网络上完整使用**：

1. **`VATSIMAuth` 专有库**：闭源，第三方无法获取，整个 VATSIM 认证握手依赖它
2. **VATSIM 数据读取器无条件运行**：无论连接到哪个服务器，始终轮询 VATSIM 基础设施
3. **AFV 语音**：无开源服务端，第三方无法自建兼容的语音后端
