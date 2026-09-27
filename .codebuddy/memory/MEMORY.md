# JPEGView 项目记忆

## SelectionZoomMode 设定项 (2026-08-08)

在 INI 配置中新增 `SelectionZoomMode` 设定项，控制框选释放后的行为：
- `false`（默认）: 裁剪模式 — 弹出裁剪菜单
- `true`: 局部放大模式 — 框选区域放大到全屏

### 改动范围（4 文件）

| 文件 | 改动 |
|------|------|
| `SettingsProvider.h` | 新增 `SelectionZoomMode()` getter + `m_bSelectionZoomMode` 成员 |
| `SettingsProvider.cpp` | `ReadWriteableINISettings()` 中 `GetBool(_T("SelectionZoomMode"), false)` |
| `MainDlg.cpp` | `OnLButtonUp` 判断中加入 `CSettingsProvider::This().SelectionZoomMode()` 条件 |
| `Config/JPEGView.ini.tpl` | 新增 `SelectionZoomMode=false` 设定项及中英文注释 |

### 交互规则
- Shift+框选 始终进入 Zoom 模式（不受 Setting 影响）
- 放大后可用 `NavigatorBack()` 返回前一视图
- 右键菜单 / 键盘裁剪命令（`IDM_CROP_SEL`）独立于模式，两种模式下均可用

### 防误触：最小框选尺寸阈值 (2026-08-08)
- `OnLButtonUp` 中取 `GetScreenCropRect()` 检查宽/高 ≥ 12px
- 不足时回退为裁剪菜单模式，防止单击抖动触发"单点放大"
- `CCropCtl::GetScreenCropRect()` 已从 private 提为 public

### 连续框选放大 (2026-08-08)
- `OnLButtonDown` 进入选择模式的条件中加入 `|| CSettingsProvider::This().SelectionZoomMode()`
- 确保 SelectionZoomMode=true 时，即使图片大于窗口、无 Ctrl、非 DefaultSelectionMode，也能连续框选放大

### 单点/微拖拽不再误弹裁剪菜单 (2026-08-08)
- `OnLButtonUp` 中，bSelectZoom=true（Shift 或 SelectionZoomMode）且选区 < 12px 时直接 `AbortCropping()` 静默取消
- 避免 2~12px 的微拖拽因 `PointDifferenceSmall`（<2px）与 zoom 阈值（12px）之间的缝隙而弹出裁剪菜单
- 普通裁剪模式行为不变

### 框选放大上限 100% (2026-08-08)
- `ZoomToSelection()` 中，当 `GetZoomParameters` 算出的 fZoom > 1.0 时，上限为 1.0（100%/1:1 像素映射）
- 同时重算 offsets 将选区居中显示（不放大超过 100%）
- 更高比例需用 Ctrl+滚轮（原逻辑不变）
- 编译通过 0 error

### >=100% 时左键进入拖拽平移 (2026-08-08)
- `OnLButtonDown` 中 `SelectionZoomMode()` 条件加 `m_state.m_dZoom < 1.0` 门控
- 缩放 >= 100% 时跳过选区模式，自然落入 `bDraggingRequired → StartDragging` 平移
- 缩放 < 100% 时 SelectionZoomMode 行为不变（框选放大）
- 编译通过 0 error

### 编译
- x64 Release 通过, 0 error 0 warning
- 类名: `CSettingsProvider::This()`（注意 C 前缀）

## ImageLoadThread 后处理管线统一 (2026-08-10)

提取 `WrapDecodedPixelsToImage` 静态辅助函数（`ImageLoadThread.cpp:469-490`），统一 6 种格式（WEBP/PNG/JXL/AVIF/HEIF/QOI）的 AlphaBlend → CJPEGImage 构造 → EXIF 释放流水线。

### 设计原则
- **不动 Wrapper 接口**：格式特有参数（chromoSubsampling、half_size、scale_denom）完全保留
- **不动文件读取逻辑**：各格式缓存管理/内存分配/错误处理差异不受影响
- **仅消除重复后处理**：~30 行重复代码压缩为 1 个函数调用

### 改动细节
- PNG hardcoded `4` (nChannels) → `nBPP`：修正隐式假设，使用解码器实际返回值
- QOI 新增 `void* pEXIFData = NULL` 局部变量以适配统一接口
- PNG GDI+ fallback 的 `free(pEXIFData)`（原 727 行）不受影响：helper 中 `free(NULL)` 是 no-op

### 新增格式接入
旧流程：写 Wrapper → 枚举 → 魔数 → switch case → 手写 ProcessRead*（AlphaBlend+构造+EXIF）~30 行
新流程：写 Wrapper → 枚举 → 魔数 → switch case → 一行 `WrapDecodedPixelsToImage(...)`

### 编译
- x64 Release 0 error（warning 均为既有 C4244 __int64 截断）

## exiv2 集成（CEXIFReader 完全替换自研解析）(2026-08-14)

- **范围**：`EXIFReader.h/.cpp` 用 exiv2 1.0（静态 /MT）替换手写 TIFF/EXIF 解析；`SaveImage.cpp` 偏移改 `GetEXIFSize()`；`JPEGView.vcxproj` 仅 Release|x64 加 `JPEGVIEW_EXIV2` 宏 + exiv2 包含/库路径 + 6 个链接库；`build-exiv2.bat` 补产物复制。x86/WinXP 构建不定义宏 → 空实现 no-op（写操作不生效、块保持原样）。
- **exiv2 1.0 API 要点**：`ExifParser::decode(ExifData&, const byte*, size_t)` 解析纯 TIFF 块；`encode(Blob&, const byte*, size, ByteOrder, ExifData&)` 返回 wmNonIntrusive（就地改、块长不变）/wmIntrusive（blob 为纯 TIFF）；`ExifThumb::setJpegThumbnail` 需要**含 SOI 完整 JPEG 流**；`ExifKey` 完整定义在 **tags.hpp**；`Exifdatum::value()` 返回 `const Value&`（要判空/取值用 `getValue()` 返回 `unique_ptr<Value>`）；Windows min/max 宏需在 include 前 `#undef`；链 `psapi.lib`（version.cpp 用 EnumProcessModules）。
- **产物**：`src/JPEGView/exiv2/`（lib64: exiv2.lib/brotlicommon/brotlidec/brotlienc/libexpatMT；include/exiv2: 全部头 + exv_conf.h + exiv2lib_export.h）。exiv2.lib 依赖 zlib/expat/brotli（zlib 复用 libpng-apng/lib64/zlib.lib，已链）。
- **保存路径**：写操作只标 dirty，`GetEXIFSize()` 触发延迟 `Reencode()`；预留增长缓冲 32000 字节（SaveImage 分配 `nJPEGStreamLen + EXIFLen + 32000`），超预算回退重编码（无缩略图版）或保持原块。
- 编译验证：x64 Release 0 error 0 warning，exe ~5.9MB。

## 与 JPEGView_L fork 的技术路线差异（长期结论，2026-09-13 调研）

参考仓库：`e:\SharedLibs\projects\player_plugin\JPEGView_L-1.4.0.5`（KrokusPokus fork，1.4.0.5）。

| 维度 | 本 fork | JPEGView_L |
|------|---------|------------|
| 重采样 SIMD | **int16 定点**（`_mm256_mulhi_epi16`，16px/256bit） | float32（`_mm256_mul_ps`，8px/256bit）+ 线性光 + `LinRGB12_sRGB8[]` 查表 |
| 像素转换 | 移位 `(px+42)>>6` | 查表 |
| HEIF 解码 | C API + `num_codec_threads`（多线程、HDR 选项） | 旧 C++ API，无线程 |
| JXL 双释放 | 已修（cache 内部拷贝） | 未修（DeleteCache 不 free 的变通实现） |
| 大图溢出保护 | 有 `(size_t)` 转换 | 无 |
| JPEG 降采样解码 | 有（Oversized/FastFit） | 无 |

**结论：不要为"性能"整体照搬 L**——其 float 路线是为线性光画质（改变输出明暗），热路径吞吐不如本 fork 的 int16 定点路线；L 的多数差异属新功能或画质改变。可借鉴的纯优化已取用：AVIF `avifDecoderCreate` 判空、JPEG ICC 由 lcms2 处理（避免 `UseEmbeddedColorProfiles` 时全量走 GDI+）、JXL 动画真实帧数（`CountJxlFrames`，修掉"只播前 2 帧"的缺陷）。

**动画多帧约定（重要）**：`Helpers::GetFrameIndex()` 用 `CJPEGImage::NumberOfFrames()` 回绕，因此任何格式返回的 `frame_count` 若是占位/近似值，会直接导致动画只播前 N 帧。新格式接入时必须返回真实帧数。JXL 无帧数元数据 → 首次解码时扫描一次并缓存（`JxlReader::cache.total_frames`）。

**尚未采纳的候选优化**（评估见 2026-09-13 记录）：线性光重采样（float，代价大、改变观感，建议做成默认关的 INI 开关）、更多下采样滤镜（成本低，中低优先级）、CacheRange（中）、SmoothPanning/漫画包/书签（低）。

## 线性光重采样开关（2026-09-13 实现）
- INI：`LinearLightResampling`（默认 **false**，仅 x64 + AVX2 生效；SSE/32 位下强制失效）。
- 实现要点：`CXMMImage` 按设置切换 float32/int16 双布局（GetLineSize ×4/×2），`ResizeFilter` 生成 float/int16 kernel（同一内存布局），`ApplyFilter_AVX_Linear` 做 float 内积 + 12bit clamp/round，`BasicProcessing` 新增 `*Linear` 版旋转/输出（用 `LookupTables.h` 的 `sRGB8_LinRGB12[256]`/`LinRGB12_sRGB8[4096]`）。
- 开关切换后 kernel 缓存通过 `CResizeFilter::ParametersMatch` 中的 `m_bLinear` 比较自动失效。

## 配置文件与验证环境（重要）
- **生效配置：`%APPDATA%\JPEGView\JPEGView.ini`**（ANSI/无 BOM）。`src\JPEGView\bin\x64\Release\JPEGView.ini` 不存在，改它无效。
- 配置项需重启 JPEGView 才生效。
- **无法用工具环境自动验证 GUI 行为**：后台 `start` 启动 JPEGView 会在数秒内自行退出（与开关状态无关，属启动环境限制）。验证需用户手动进行。
- 既有无害现象：调试器下两种配置都会报 `CMainDlg::PerformZoom+0x468` 的 first-chance AV（可恢复，与重采样改动无关）。

## 新增快捷键/命令的完整清单（项目约定）
按键切换类功能（参考 `IDM_FASTFIT_SCREEN_DECODE`/F 键）需要改 6 处：
1. `resource.h`：`#define IDM_xxx <id>`，带 `// :KeyMap: <描述>` 注释（ID 取未占用值，注意不要与既有冲突）。
2. `SettingsProvider`：需要会话级开关时加 `Override` 三件套（Possible/SetOverride/Clear），getter 优先返回覆盖值。
3. `MainDlg.cpp` `ExecuteCommand`：新增 `case`，切换后若影响显示需让处理缓存失效（`m_pCurrentImage->SetDIBInvalid()`）再 `Invalidate(FALSE)`；必要时 `UpdateWindowTitle()` 显示状态（已有 `[fast]`/`[lin]`/`[1/N fit]` 风格）。
4. `MainDlg.cpp` 右键菜单：`::CheckMenuItem(...)` 反映当前状态。
5. `JPEGView.rc`：菜单项（PopupMenu 资源）。
6. **键表两处 + 符号表两处**：`bin\x64\Release\KeyMap.txt`（**实际生效**，优先于 default）+ `Config\KeyMap.txt.default`；`bin\x64\Release\symbols.km` + `Config\symbols.km`（**漏了 symbols 会导致键位静默失效**）。
变更键表/符号表**不需要重新编译**，但改 rc/代码需要重新构建。

## 动画（多帧）格式支持现状（2026-09-20 调研）
| 格式 | 动画支持 | 帧数来源 | 备注 |
|------|---------|---------|------|
| AVIF | ✅ 完整 | libavif `decoder->imageCount`（直接可得） | 逐帧 `avifDecoderNthImage`，动画时保留 decoder 缓存 |
| HEIC/HEIF | ❌ 不支持（只显示首帧） | `get_number_of_top_level_images()` = 多图容器，非动画帧 | ImageLoadThread 硬编码 `nFrameCount=1`、`bHasAnimation=false`；未接入 `heif_sequences.h` |
| JXL | ✅（已修真实帧数） | 首次扫描缓存 `cache.total_frames` | 见 JXL 帧数修复 |
| WebP / GIF / PNG(APNG) | ✅ | 各自库提供 | — |

- libheif 1.23.1 **已具备**序列 API（`heif_context_has_sequence`、`heif_context_get_track`、`heif_track_decode_next_image`、`heif_image_get_duration`），支持 HEIC 动画**不需要升级库**。
- HEIC 动画的主要障碍：libheif **没有总帧数 API**，且 `heif_track_decode_next_image` 是**顺序解码**（无随机 seek）；预扫描数帧需完整解码一遍，代价大。
- **动画通用约束（重要）**：`Helpers::GetFrameIndex()` 用 `CJPEGImage::NumberOfFrames()` 做回绕判定 → 任何格式若返回占位帧数，动画只会播前 N 帧（JXL 曾因此只播 2 帧）。新增动画格式必须给出真实帧数，或改造回绕机制。
- **WebP 动画实现**（`WEBPWrapper.cpp`）：`WebPAnimDecoder` + `WebPAnimDecoderGetNext()` **顺序推进**（`ImageLoadThread` 调用时**不传 frame_index**），末尾 `HasMoreFrames()` 为假时 `WebPAnimDecoderReset()` 回绕；帧数取 `WebPAnimDecoderGetInfo().frame_count`（真实值）；帧时长用相邻 timestamp 差值。已知弱点：①顺序推进与"按 FrameIndex 请求 + 请求缓存"不严格匹配（缓存时序异常时可能错帧/卡帧，无跳帧能力）；②回绕后首帧 `timestamp < prev` 分支把 prev 置 0 → 该帧 frame_time=0（可能快速闪过）。
- **GIF 动画实现**（走 GDI+，`ImageLoadThread.cpp` 的 `ConvertGDIPlusBitmapToJPEGImage`）：`GetFrameCount(FrameDimensionTime)` 取真实帧数、`SelectActiveFrame()` **随机访问**、帧时长取 `PropertyTagFrameDelay[nFrameIndex]*10`（GDI+ 单位 1/100s）、帧合成（disposal）由 GDI+ 负责、`bIsAnimation` 仅对 IF_GIF 置真。缺 Netscape 循环次数语义（统一按 JPEGView 自身循环策略）；解析依赖 GDI+（大 GIF 较慢）。
- **APNG 也属于"顺序流"家族**（`PNGWrapper.cpp`）：`ReadNextFrame()` 内部 `cache.frame_index++ %= frame_count`，回绕靠 `cache.frame_index == 0` 时重建解码器（`DeleteCacheInternal` + `BeginReading`）；**同样不接收请求帧号**。帧数来自 `acTL`（真实值），帧时长 = `delay_num/delay_den`。
- **动画实现三模型**：①随机访问 = GIF（GDI+ `SelectActiveFrame`）、AVIF（`avifDecoderNthImage`）；②顺序流 + 重置 = WebP（`WebPAnimDecoderGetNext`/`Reset`）、JXL（顺序推进 + `JxlDecoderRewind`）、APNG（`ReadNextFrame`/重建）；③不支持 = HEIC/HEIF。
- **关于"顺序流格式错帧"的最终结论（2026-09-20 修订）**：`CJPEGProvider::FindRequest(file, FrameIndex)` 命中缓存时确实不会调用 `ReadImage`，但 **`MainDlg.cpp:68-69` 定义 `NUM_THREADS = 1`、`READ_AHEAD_BUFFERS = 2`**，且 `RemoveUnusedImages()` 的 `nMaxListSize = m_nNumBuffers - 1 = 1` → 请求列表实际只保留 **1~2 个**条目。因此动画每帧几乎总是缓存未命中 → 每次都真实解码 → **顺序请求与顺序解码器天然同步，正常播放路径下没有可复现的错帧**。此前"错帧"判断是在假设"缓存保留全部帧"的前提下做的推断，与实际配置不符，**已收回**。
- 仍存在的真实边界风险（条件触发，非现网 bug）：①若把 `READ_AHEAD_BUFFERS` 调大（例如为提升性能改成 8/16），缓存命中与未命中会交错 → 立刻出现错帧，即该正确性依赖"缓存极小"这一配置巧合；②`bWasOutOfMemory` 重试路径（`FreeAllPossibleMemory` → 同一 FrameIndex 重新解码）可能让解码器多前进一帧 → 错帧（需 OOM 才触发）；③无跳帧能力（UI 也未提供跳帧入口）。
- **改进路线（评估，未实施）**：A. `ReadImage` 接收 `frame_index` + cache 记 `next_frame_index`，失配时 Reset 后重放到目标帧（常态零开销，~150 行覆盖 WebP/JXL/APNG）；B. 抽 `IAnimationFrameSource` 统一契约（含"未知帧数 + EndOfSequence 回绕"模式，可顺带支持 HEIF），~400-600 行；C. 专用流式播放管线（解码线程 + 环形预解码缓冲），体验最优但需与缩放/处理管线共存，工作量大。
- 帧时间补偿本项目**已有**（`AppState::m_nExpectedNextAnimationTickCount` + MainDlg 的 `nRealDisplayTimeMs`），无需借鉴上游 PR #205。

## 同类软件动画路线调研（2026-09-20）
- **JarkViewer**（`jark006/JarkViewer`，C++/Win32，GPL-3，1.3k★）：动画支持 6 种 = gif / png / **apng** / webp / **jxl** / **avif**；**HEIC 动画不支持**（HEIC/HEIF 只列在静态与"实况照片"）。依赖含 **ffmpeg**（静态）、OpenCV、libavif、libjxl、libheif、libyuv；交互为 J/K/L 逐帧、空格播放暂停、Ctrl+S 拆帧导出、顶部控制栏。CSDN 文章（**AIGC，仅供参考**）称其架构为"解码器 → 帧缓存 → 渲染层"、线程池并行解码、内存驻留型帧缓存；文中点名入口文件 `src/main.cpp`（帧控制）+ **`videoDecoder.cpp`（动图帧解码）**。→ 结合 ffmpeg 依赖与文件名，**高度可能是"把动图交给 FFmpeg 视频解码管线 + 统一帧模型"**（GitHub raw/blob 抓取超时，未获源码直接证实）。
- **IrfanView**：官方 history 显示 **4.76（2026-09-18）才加入 "Support for animated AVIF files"**；HEIC 仅静态（FAQ 要求装系统 HEIF/HEVC 扩展或第三方 codec，未提动画）；GIF 内置；其余动画支持散落在插件更新、无统一说明。路线 = 逐格式插件 + 内部"多页/多帧"框架，多年逐个补课。
- **行业结论**：①**HEIC 动画普遍缺失**（IrfanView、JarkViewer 均无）；②新格式动画各家都是逐个补（IrfanView 2026-09 才补 AVIF 动画）；③存在两条路线：**视频流统一**（JarkViewer：ffmpeg demux/decode + 帧模型 + 定时播放）vs **逐格式实现 + 统一多页框架**（IrfanView、本项目）。
- **本项目定位**：动画覆盖 = GIF / APNG / WebP / AVIF / JXL（与 JarkViewer 持平，且在 animated AVIF、JXL 动画上**早于 IrfanView**）；优势为 lcms2 色彩管理 + 与处理管线集成；短板 = 顺序流帧号契约缺失（错帧隐患，见上文）、无统一动画抽象、HEIC 动画缺失。
- **建议（未实施）**：走统一帧源抽象（方案 B），**不引入 ffmpeg**（静态体积与色彩管理/处理管线对接代价过大）；若要做"人无我有"，用 libheif 1.23 的 sequence API 实现 HEIC 动画，并由方案 B 的"未知帧数 + 顺序推进 + EndOfSequence 回绕"契约承载。

## 本 fork 关键默认值
- `UseEmbeddedColorProfiles` 默认 **false**（SettingsProvider.cpp:214）；开启后 JPEG 于 2026-09-13 起由 lcms2 处理 ICC（不再走 GDI+），TurboJPEG 快速路径与降采样解码保持可用。
- `ForceGDIPlus` 默认 false；JPEG 仅在它为 true 时走 GDI+。
- `OversizedDownscaleDecode` 默认 true、`FastFitScreenDecode` 默认 false、`FastJPEGDecode` 默认 false、`SingleInstance` 默认 PerFolder。
- `FilesProcessedByWIC` 默认 `*.wdp;*.mdp;*.hdp`，`FileEndingsRAW` 默认 pef/dng/crw/nef/cr2/mrw/rw2/orf/x3f/arw/kdc/nrw/dcr/sr2/raf。

## 格式支持清单与候选新增（2026-09-20 调研）
**已支持**：JPEG(jpg/jpeg/jfif)、BMP、PNG(含 APNG)、TIFF(含多页)、GIF(动)、WEBP(动)、JXL(动)、AVIF(动)、HEIF/HEIC/HIF(仅静态)、TGA、QOI、PSD/PSB、DDS、JPEG XR(wdp/mdp/hdp，经 WIC)、RAW(dcraw 十余种)。
- 扩展名内置列表见 `FileList.cpp:130-132`（18 项）；扩展名→格式映射见 `Helpers.cpp:756-778`。

**缺口（按价值/成本排序）**
1. **零成本补漏（各 1 行，加入 FileList 扩展名列表 / Helpers 映射）**：`.apng`（APNG 普遍用独立扩展名，当前打不开）、`.avifs`（AVIF 序列）、`.hif`（Helpers 已认但列表缺）、`.psb`（列表有但 Helpers 无映射，可能走 WIC 失败）。
2. **HEIC/HEIF 动画**：IrfanView、JarkViewer 均不支持 → **差异化机会**；libheif 1.23.1 已具备 sequence API（`heif_context_has_sequence`/`heif_context_get_track`/`heif_track_decode_next_image`/`heif_image_get_duration`），但**无总帧数 API 且顺序解码无 seek** → 需"未知帧数 + 顺序推进 + EndOfSequence 回绕"契约。成本中等。
3. **SVG**（矢量素材/图标场景常用）：需渲染库（lunasvg 或 Direct2D），按显示尺寸栅格化反而更清晰。成本中等。
4. **OpenEXR / Radiance HDR(.hdr)**：HDR 图像；`tinyexr`/`stb_image` 单头文件即可；可与已有 HDR→8bit 显示路径衔接。成本中等。
5. **Netpbm(pbm/pgm/ppm/pnm)**：纯手工解析约几十行、零依赖；面向开发/科研，成本极低但受众小。
6. **JPEG 2000(jp2/j2k)**：需 OpenJPEG，成本中高；仅专业/档案场景。
7. **KTX2/ASTC（游戏纹理）**、**Live Photo/Motion Photo(.livp)**（需视频轨解码 → ffmpeg/MF，成本高）、**ICO/CUR/ANI**：优先级低。

**新增一个格式的真实工作量**（不止解码器）：解码 wrapper + magic 检测 + 扩展名三处（FileList / Helpers / FileExtensionsDlg）+ ICC 色彩管理接入 + **多帧则必须给真实帧数**（否则重演 JXL"只播 2 帧"）+ EXIF（可选）+ 降采样解码（可选）+ 安装脚本 SupportedTypes + NLS 文案 + 回归测试。

## CI / 工作流现状（2026-09-27）
- **GitHub 侧**：`.github/workflows/*`（8 个，源自上游 sylikc/JPEGView）**从未运行过**（API `total_count=0`）。按用户要求**保留不动**。已知失效点：依赖不存在的 `src/JPEGView.Setup`、`workflow-build-wtl-cache`/`workflow-rebuild-all-deps` 引用已删的 `setup-wix`、`bin-cache` 会 `del /s *.lib *.dll` 删掉我们的预编译库、runner/action 版本老旧（windows-2019 / @v3）。
- **Gitea 侧**：实例 `git.naspg.cn` = Gitea **1.27.2**；新增 `.gitea/workflows/build.yml`（自包含：仅 `runs-on: windows-latest` 单标签 + matrix + 内联脚本，不用 reusable workflow / 本地 action / cache / artifact）。旧版 GitHub workflow 一行未动。
- **构建适配事实**：`src/JPEGView.sln` 存在（JPEGView + WICLoader）；`JPEGView.vcxproj` 已内置 `deps/WTL-sf\Include` → CI 只需 `git submodule update --init deps/WTL-sf`；**32 位预编译库缺失（只有 lib64）→ 只构建 Release|x64**；本机无 vswhere，VS 18 在 `D:\Program Files\Microsoft Visual Studio\18\Community`。
- **Gitea 兼容边界**：工作流目录 `.gitea/workflows/`；`runs-on` 仅支持 `xyz`/`[xyz]`；表达式仅保证 `always()`；`workflow_call`/本地 composite action/cache/artifact 文档均未确认 → 兼容写法须用共同子集。

## 上游 JPEGView_L 修复跟踪（核对至 v1.4.0.6，2026-09-26）
上游最新 **v1.4.0.6**（2026-09-24，`9096d96`）。v1.4.0.5→v1.4.0.6 实质只动 4 个源文件，逐项适用性：
- **已实施（2026-09-26）Fix #21 文件大小加小数**：上游改用 `FormatFileSize()`（KiB/MiB 单位）。**我们按用户要求保留 MB/KB 单位、统一两位小数**（`%.2f MB` / `%.2f KB`，<1 KiB 仍为 `%d b`）：`Helpers.cpp` 中 `GetFileInfoString` 之前新增 static `FormatFileSize()`，其 `<l>` 分支改调用它。注意：与上游实现的**单位与精度都不同**，日后同步上游时不要直接覆盖。
- **未采用（2026-09-26 实施后回退）Fix #8**：`TJPEGWrapper.cpp:170` 保持原 int 表达式 `TJPAD(nScaledWidth * 3) * nScaledHeight`。曾改为 `static_cast<size_t>` 并编译通过，随后按用户"最小 diff"原则回退（守卫 `MAX_IMAGE_PIXELS` 下溢出不可达，属纯防御性）。判断依据：溢出阈值 715.8 Mpx vs 守卫 524.3 Mpx，余量 26.7%。
- [最小 diff 原则（用户偏好）](feedback_minimal_diff.md) — 当前不可达的防御性改动不进代码库
- **不适用**：Fix #14 滚轮失效（我们 `OnMouseWheel` 实现不同，无条件 GotoImage）、Fix #3 全屏误弹缩放因子（我们不设 `m_bInZooming`）、Fix #9 面板定时器嵌套、zip 归档加载（我们无归档功能）。
- **不采用**：`IDM_CUT_FILE`/`IDM_RELOAD_FULL`/阿拉伯语（新功能）、`DisplayFullSizeRAW` 默认 0→3（行为改变）、`ApplyFilter_AVX` "重构"（实为 tab→空格格式化，零逻辑变化）。

**调研方法提示**：本机 `JPEGView_L-1.4.0.5` 副本**不是 git 仓库**，无法 git fetch/log；用 GitHub API（`/releases`、`/commits/{sha}`、`/compare/{base}...{head}`）+ curl 下载 JSON，再用 PowerShell `ConvertFrom-Json` 脚本解析（cmd 下引号与 `<`、`$` 易被吞，用脚本文件规避）。
