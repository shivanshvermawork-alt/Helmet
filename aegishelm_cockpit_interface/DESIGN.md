---
name: AegisHelm Cockpit Interface
colors:
  surface: '#faf8ff'
  surface-dim: '#d2d9f4'
  surface-bright: '#faf8ff'
  surface-container-lowest: '#ffffff'
  surface-container-low: '#f2f3ff'
  surface-container: '#eaedff'
  surface-container-high: '#e2e7ff'
  surface-container-highest: '#dae2fd'
  on-surface: '#131b2e'
  on-surface-variant: '#434655'
  inverse-surface: '#283044'
  inverse-on-surface: '#eef0ff'
  outline: '#737686'
  outline-variant: '#c3c6d7'
  surface-tint: '#0053db'
  primary: '#004ac6'
  on-primary: '#ffffff'
  primary-container: '#2563eb'
  on-primary-container: '#eeefff'
  inverse-primary: '#b4c5ff'
  secondary: '#006c4a'
  on-secondary: '#ffffff'
  secondary-container: '#82f5c1'
  on-secondary-container: '#00714e'
  tertiary: '#ac0031'
  on-tertiary: '#ffffff'
  tertiary-container: '#d71142'
  on-tertiary-container: '#ffecec'
  error: '#ba1a1a'
  on-error: '#ffffff'
  error-container: '#ffdad6'
  on-error-container: '#93000a'
  primary-fixed: '#dbe1ff'
  primary-fixed-dim: '#b4c5ff'
  on-primary-fixed: '#00174b'
  on-primary-fixed-variant: '#003ea8'
  secondary-fixed: '#85f8c4'
  secondary-fixed-dim: '#68dba9'
  on-secondary-fixed: '#002114'
  on-secondary-fixed-variant: '#005137'
  tertiary-fixed: '#ffdada'
  tertiary-fixed-dim: '#ffb3b6'
  on-tertiary-fixed: '#40000c'
  on-tertiary-fixed-variant: '#920028'
  background: '#faf8ff'
  on-background: '#131b2e'
  surface-variant: '#dae2fd'
typography:
  headline-xl:
    fontFamily: Space Grotesk
    fontSize: 48px
    fontWeight: '700'
    lineHeight: 56px
    letterSpacing: -0.03em
  headline-xl-mobile:
    fontFamily: Space Grotesk
    fontSize: 32px
    fontWeight: '700'
    lineHeight: 40px
    letterSpacing: -0.02em
  headline-lg:
    fontFamily: Space Grotesk
    fontSize: 36px
    fontWeight: '600'
    lineHeight: 44px
    letterSpacing: -0.02em
  headline-lg-mobile:
    fontFamily: Space Grotesk
    fontSize: 26px
    fontWeight: '600'
    lineHeight: 34px
    letterSpacing: -0.02em
  headline-md:
    fontFamily: Space Grotesk
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
    letterSpacing: -0.01em
  headline-sm:
    fontFamily: Space Grotesk
    fontSize: 20px
    fontWeight: '500'
    lineHeight: 28px
  body-lg:
    fontFamily: Geist
    fontSize: 18px
    fontWeight: '400'
    lineHeight: 28px
  body-md:
    fontFamily: Geist
    fontSize: 15px
    fontWeight: '400'
    lineHeight: 22px
  body-sm:
    fontFamily: Geist
    fontSize: 13px
    fontWeight: '400'
    lineHeight: 18px
  label-lg:
    fontFamily: JetBrains Mono
    fontSize: 14px
    fontWeight: '600'
    lineHeight: 20px
    letterSpacing: 0.04em
  label-md:
    fontFamily: JetBrains Mono
    fontSize: 12px
    fontWeight: '500'
    lineHeight: 16px
    letterSpacing: 0.06em
  label-sm:
    fontFamily: JetBrains Mono
    fontSize: 10px
    fontWeight: '500'
    lineHeight: 14px
    letterSpacing: 0.08em
  telemetry-unit:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: '700'
    lineHeight: 14px
    letterSpacing: 0.1em
rounded:
  sm: 0.5rem
  DEFAULT: 1rem
  md: 1.5rem
  lg: 2rem
  xl: 3rem
  full: 9999px
spacing:
  gutter: 1.5rem
  gutter-mobile: 1rem
  margin: 2rem
  margin-mobile: 1rem
  space-xs: 0.25rem
  space-sm: 0.5rem
  space-md: 1rem
  space-lg: 1.5rem
  space-xl: 2.5rem
---

## Brand & Style

This design system establishes a high-performance, ultra-luxury automotive telemetry experience inspired by Scandinavian electric vehicle cockpits (Polestar) and Cupertino interface restraint (Apple). Tailored for embedded IoT vehicular telemetry and advanced rider-assistance systems, it bridges high-stakes safety engineering with tactile digital elegance.

### Emotional Resonance & Tone
- **Calm Authority:** Eliminates cognitive overload under rapid motion while conveying complete, deterministic control.
- **Instrument Precision:** Every metric, warning, and vector reflects laboratory-grade sensor accuracy.
- **Tactile Luxury:** Soft porcelain surfaces, diffuse optical diffusions, and micro-radii elevate the experience above utilitarian hardware dashboards.

### Design Movement: Modern Automotive Neomorphism & Glassmorphism Hybrid
The visual language merges high-clarity minimalism with optical depth:
- **Porcelain Grounding:** Replaces hyper-dark cyberpunk cliches with a refined, soft slate porcelain backdrop that minimizes eye fatigue across changing daylight conditions.
- **Optic Glow Layers:** Critical state transitions leverage sub-surface radial blooms—sapphire for active link state, emerald for powertrain safety, amber for environmental warnings, and coral rose for immediate collision/fall emergency states.

## Colors

The palette establishes an ultra-refined light automotive cockpit environment engineered for high daylight legibility, rapid state recognition, and minimal ocular glare.

### Key Palettes & Roles
- **Base Canvas (`#F8FAFC` - Porcelain Slate):** The non-glare, ultra-soft foundation that reduces luminance fatigue while maintaining modern digital warmth.
- **Primary Telemetry (`#2563EB` - Sapphire Core):** Powers dynamic vehicle metrics, active BLE helmet tether states, heads-up display links, and primary interactive surfaces.
- **Secondary Telemetry (`#059669` / `#10B981` - Emerald Shield):** Validates safe subsystem diagnostics: nominal tire pressure, helmet chin-strap latch confirmation, full ECU connectivity, and optimal battery reserves.
- **Critical Emergency (`#E11D48` - Coral Rose):** Reserved strictly for high-severity events: impact detection, tilt-angle rollover thresholds, emergency beacon broadcasts, and immediate brake override alerts.
- **Hazard Warning (`#D97706` - Amber Vector):** Denotes non-critical maintenance alerts, degraded sensor calibration, crosswind warnings, and low-traction warnings.
- **Deep Slate Typography (`#0F172A`):** Pure optical contrast against porcelain surfaces, ensuring instant glanceability without harsh pure black clipping.
- **Structural Outlines (`#E2E8F0`):** Defines mechanical containment boundaries for instrumentation modules.

## Typography

The typographic hierarchy is split across three distinct design disciplines to reinforce engineering authority:

- **Display & Section Anchors (`Space Grotesk`):** Technical, structural, and architectural. Used for critical mode headings, speed clusters, and primary telemetry numbers to evoke bespoke automotive gauges.
- **Interface & Operational Prose (`Geist`):** Clean, neutral, and hyper-legible. Handles system logs, rider status diagnostics, settings, and descriptive guidance without visual friction.
- **Embedded Telemetry & System Gauges (`JetBrains Mono`):** Tabular, monospaced precision. Applied to live sensor telemetry streams (angles, barometric pressure, ECU voltages, millisecond latency, GPS coordinates), ensuring numbers never jitter or jump across dynamic updates.

## Layout & Spacing

The dashboard applies a cockpit-derived 12-column modular grid designed for widescreen automotive center consoles (touchscreen head units) and reflows seamlessly to companion smartphone displays.

### Layout Mechanics
- **Desktop / Automotive Landscape Screen (≥ 1024px):** 12-column grid structure with 24px (`1.5rem`) gutters and 32px (`2rem`) outer cockpit margins. Left rail (3 columns) hosts real-time helmet & rider telemetry; center stage (6 columns) contains dynamic speed, lean-angle attitude indicators, and collision maps; right rail (3 columns) prioritizes environmental conditions, hardware battery levels, and safety interlocks.
- **Tablet / Secondary Display (768px - 1023px):** 8-column layout. Central gauge collapses into an elevated horizontal instrument cluster; auxiliary readouts dock below into 4-column parity cards.
- **Mobile Companion Mode (< 768px):** Single-column stacked architecture with 16px (`1rem`) outer margins. Critical status banners float pinned to top safe areas, placing SOS panic triggers within direct thumb reach.

## Elevation & Depth

Visual hierarchy combines physical surface boundaries with optical sensor illumination:

### Elevation Hierarchy
1. **Base Layer (Level 0 - Ground):** The `#F8FAFC` porcelain background provides absolute clarity. No cast shadows.
2. **Cockpit Instrument Modules (Level 1 - Telemetry Cards):** Background `#FFFFFF` with 1px structural borders tinted in `#E2E8F0`. Elevation is conveyed via ultra-diffused atmospheric ambient drop shadows: `0px 10px 30px -4px rgba(15, 23, 42, 0.04), 0px 4px 12px -2px rgba(15, 23, 42, 0.02)`.
3. **Floating Controls & Active Modals (Level 2):** Elevated interactive components cast a deeper ambient layer: `0px 20px 40px -8px rgba(15, 23, 42, 0.08), 0px 8px 16px -4px rgba(15, 23, 42, 0.03)`.
4. **Active Optical Radial Glows (Telemetry State Lighting):**
   - **Nominal Link (Sapphire):** Cards with active helmet streaming emit a soft interior radial gradient from the top right edge: `radial-gradient(circle at 90% 10%, rgba(37, 99, 235, 0.08) 0%, transparent 60%)`.
   - **Protection Engaged (Emerald):** System active indicators cast `0px 0px 24px rgba(16, 185, 129, 0.25)`.
   - **Warning Vector (Amber):** Warning indicators emit `0px 0px 24px rgba(217, 119, 6, 0.28)`.
   - **Emergency Impact (Coral Rose):** High-priority modules pulse with an ambient red field: `0px 0px 32px rgba(225, 29, 72, 0.35)`.

## Shapes

The design system adopts an architectural rounded geometry that matches modern car console hardware enclosures:

- **Telemetry Cards & Stage Enclosures:** Styled with `rounded-3xl` (24px to 32px border radius), softening industrial electronics into a premium, sculpted consumer dashboard.
- **Pills & Status Badges:** Fully circular/pill geometry (`rounded-full`) for high-contrast warnings, network status nodes, and rapid-scan telemetry tags.
- **Controls & Switches:** Sub-elements inherit generous squircle profiles (`rounded-2xl` for inputs/touch targets) providing comfortable target zones during vehicular movement.

## Components

### 1. Telemetry Cards (Rounded-3xl Modules)
- **Base Surface:** Solid `#FFFFFF` fill with `rounded-3xl` (28px radius) and 1px `#E2E8F0` hairline border.
- **Sub-header Bar:** Monospaced label in `JetBrains Mono` (`label-md`, `#64748B`), capitalized with `0.08em` tracking, paired with an icon or live pulse ping indicator.
- **Content Staging:** Spacious interior padding (`space-lg` / 24px) preventing cluster collision.

### 2. Status Badges & IoT Indicators
- **Structure:** Pill shape (`rounded-full`), height of 28px, inner padding `0.25rem 0.75rem`.
- **Nominal Safe Badge:** Background `rgba(5, 150, 105, 0.1)`, text `#059669`, 1px border in `rgba(5, 150, 105, 0.25)`. Features a 6px pulsing emerald indicator dot.
- **Critical Alert Badge:** Background `rgba(225, 29, 72, 0.12)`, text `#E11D48`, 1px border in `rgba(225, 29, 72, 0.3)`. Emits subtle coral glow.
- **Telemetry Readout Badge:** Background `#F1F5F9`, text `#0F172A`, monospaced typography with units set in muted slate (`#64748B`).

### 3. Precision Telemetry Gauges & Dial Arc Displays
- **Arc Tracks:** Passive track rendered in 4px `#E2E8F0` stroke. Dynamic value fills sweep using vibrant linear gradients (`#2563EB` to `#60A5FA` or `#059669` to `#10B981`).
- **Numeric Centers:** Primary value rendered in `Space Grotesk` (`headline-xl`), with the scientific unit docked underneath in `JetBrains Mono` (`telemetry-unit`).

### 4. Interactive Cockpit Buttons
- **Primary Action (Engage/Tether):** Background `#2563EB`, text `#FFFFFF`, font `Space Grotesk` medium, height 48px, radius `rounded-2xl`. Soft sapphire glow on hover/active.
- **Secondary Surface:** Background `#F8FAFC`, border 1px solid `#E2E8F0`, text `#0F172A`. Active state features a 1px border highlight in `#2563EB`.
- **Emergency SOS Override:** Large pill trigger, solid `#E11D48` background, uppercase bold label, haptic feedback target size (minimum 56px height).

### 5. Input Fields & Parameter Steppers
- **Fields:** Flush `#F8FAFC` fill with 1px border in `#CBD5E1`. Rounded-xl corner radius. Focus states illuminate with 1px `#2563EB` outline and 3px diffused outer halo `rgba(37, 99, 235, 0.15)`.

### 6. Embedded Hardware Specific Modules
- **Helmet Latch & Visor Matrix:** Visual cross-section diagram displaying chin strap magnetic lock status, visor optical tint percentage, and heads-up display mirror link.
- **IMU Lean Angle Visualizer:** Horizontal attitude horizon line indicating left/right motorcycle banking angles with amber limit thresholds at > 45°.
- **Environmental & Impact Vector Tracker:** Real-time G-force accelerometer ring that logs instantaneous shocks and displays warning badges if rapid deceleration exceeds safety parameters.