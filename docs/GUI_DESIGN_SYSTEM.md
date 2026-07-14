# neoswift GUI design system

## Source design language

The portal in `web-frontend` is the visual reference for neoswift. It uses:

- Ant Design 5.16 with `theme.darkAlgorithm` selected from the operating-system color scheme.
- Ant Design Pro Components for data-heavy administration surfaces.
- Ant Design Icons, Charts, and Plots.
- A system sans-serif stack, compact 14 px body text, 6-8 px control radii, 20 px page padding, and restrained 200 ms interaction transitions.
- Portal shell colors `#141414` (background), `#1f1f1f` (surface), `#303030` (split border), and the Ant primary seed `#1677ff`.

Qt Widgets cannot consume React components directly. The equivalent component system is implemented in the shared QSS layer and uses Ant Design dark-algorithm output values where the derived interactive color differs from the seed.

## GUI inventory and coverage

The migration audited all 201 Qt Designer forms under `src`:

| Area | Forms |
| --- | ---: |
| Reusable GUI components | 153 |
| Editors | 20 |
| Filters | 10 |
| Views and shared GUI forms | 7 |
| Simulator plugins | 7 |
| swift, swiftcore, swiftdata, and launcher shells | 4 |

The most common widgets are `QLabel` (463), `QLineEdit` (304), `QPushButton` (251), `QFrame` (242), `QCheckBox` (218), and `QGroupBox` (143). The shared theme also covers dialogs, tool buttons, radio buttons, combo/spin boxes, tabs, menus, tables, trees, lists, splitters, progress bars, sliders, scroll bars, status bars, toolbars, dock widgets, wizards, message boxes, and tooltips.

The three bundled Qt Quick surfaces in `src/gui/qml` use the same colors. They are currently resource-only and have no C++ load site, but remain themed so a future caller does not reintroduce a light or legacy-colored surface.

## Theme loading

Every application shell composes the theme in this order:

1. `fonts.qss`
2. `stdwidget.qss`
3. Its application-specific QSS, such as `swiftstdgui.qss` or `swiftlauncher.qss`

Specialized info bars, navigators, filter dialogs, dock tabs, and rich-text messages load their dedicated styles through `CStyleSheetUtility`. The QSS directory is installed by `src/gui/CMakeLists.txt` and is watched during local developer builds.

## Design tokens

| Role | Qt value | Portal relationship |
| --- | --- | --- |
| Window background | `#141414` | `--app-bg` |
| Container / card | `#1f1f1f` | `--app-surface` |
| Hover / elevated fill | `#262626` | Ant dark elevated state |
| Split border | `#303030` | `--app-border` |
| Control border | `#424242` | Ant dark control border |
| Primary seed | `#1677ff` | Portal brand and Ant seed |
| Primary action | `#1668dc` | Ant dark algorithm primary |
| Primary hover | `#3c89e8` | Ant dark algorithm hover |
| Primary active | `#1554ad` | Ant dark algorithm active |
| Primary text | `#d9d9d9` | white at 85% |
| Secondary text | `#a6a6a6` | white at 65% |
| Disabled text | `#595959` | Ant dark disabled state |
| Selection | `#111a2c` | Ant dark blue surface |
| Success | `#49aa19` | Ant dark green |
| Warning | `#d89614` | Ant dark gold |
| Error | `#dc4446` | Ant dark red |

## Component mapping

| Portal component | Qt equivalent |
| --- | --- |
| `Layout`, `Card` | `QMainWindow`, `QFrame`, `QGroupBox` |
| `Button` | `QPushButton`, `QToolButton` |
| `Input`, `InputNumber`, date/time inputs | `QLineEdit`, spin boxes, date/time edits |
| `Select`, `Dropdown`, `Menu` | `QComboBox`, `QMenu` |
| `Tabs`, `Splitter` | `QTabWidget`, `QTabBar`, `QSplitter` |
| `Table`, `List`, tree data views | Qt item views and headers |
| `Modal`, `Drawer` | `QDialog`, tool dialogs, dock widgets |
| `Progress`, form validation | `QProgressBar`, validation properties |
| `Alert`, `Badge`, `Tag` | `statusRole` semantic surfaces |

Buttons follow Ant hierarchy: the default is a neutral bordered action. `QPushButton:default`, `#pb_Ok`, `#pb_Connect`, or a button with `buttonRole="primary"` is primary. Destructive actions may set `buttonRole="danger"`.

Mouse focus alone does not turn a neutral button blue. Blue borders are reserved for primary actions and explicit checked/selected state. Item-view selection uses the dark blue selection surface with normal primary text, not link-blue text.

Status labels and tool buttons set `statusRole` to `info`, `success`, `warning`, or `error`. Code that changes a dynamic property must call `CGuiUtility::forceStyleSheetUpdate()` so Qt immediately repolishes the widget.

## Icon system

The legacy bundle contains 4,320 image resources: 3,572 Diagona icons, 597 Pastel icons, 102 neoswift-owned images, 37 QLed assets, 10 VATSIM rating icons, and 2 textures. The source GUI previously referenced many Diagona and Pastel PNG files directly, bypassing `CIcons`.

Generic UI actions now use the vendored Ant Design Icons SVG set under `src/misc/icons/ant-design`, with the upstream MIT attribution retained in every file. `CIcons` is the canonical access point for actions such as save, refresh, edit, delete, close, search, filter, user, status, lock, volume, copy/paste, zoom, database, and window controls. Designer forms and QSS must use the same `/ant-design` resource prefix rather than directly referencing legacy icon packs.

Brand marks, neoswift application icons, VATSIM ratings, controller-position symbols, radio/joystick/radar imagery, voice-capability glyphs, and other aviation-specific assets remain domain resources because replacing them with a generic web icon would reduce meaning.

## Migration rules

- Do not add literal UI colors in C++ or Designer `styleSheet` properties. Add a semantic dynamic property and style it in QSS.
- Keep functional aviation states semantic: transponder states use the shared info/success/warning surfaces rather than arbitrary saturated fills.
- Prefer 6 px control radii and 8 px card/dialog radii. Dense cockpit controls may retain compact heights, but must use the same color, border, focus, hover, disabled, and selected states.
- New user-visible text remains English in source and must ship with the matching Qt translation update.
- QML surfaces must use this token table even though QSS does not apply to them.
- Do not introduce new direct references to `:/pastel` or `:/diagona` from GUI code, Designer forms, or QSS. Add or reuse a semantic Ant icon through the shared resource layer.
- A resizable `QScrollArea` must give its content widget a realistic minimum height. A Designer geometry value alone is not a constraint; without a minimum, Qt may compress group boxes and clip controls instead of showing a scroll bar.
- Avoid fixed or maximum heights below the shared control box height. When a compact fixed-height control is intentional, include its QSS padding and border in the calculation and verify it at the default application font.
- `QSlider` backgrounds stay transparent; only the groove and handle paint a surface. This prevents the generic widget surface from appearing as a rectangle behind the track.
