# DirectX 12 Fluid Simulation

DirectX 12 APIを用いた、物理シミュレーション
コンピュートシェーダを活用した流体シミュレーションを実装



https://github.com/user-attachments/assets/81dd4665-d3e5-4d31-9658-b9881ffdb077

## セットアップ方法
BuildExternal.batを起動

## 主な機能 (Features)

### 1. Fluid Simulation (SPH)
* **SPH法 (Smoothed Particle Hydrodynamics)**: 粒子法を用いた流体シミュレーションを実装。
* **Compute Shader**: 物理演算をGPU上のコンピュートシェーダで並列処理し、多数の粒子（20,000〜）をリアルタイムに制御。
* **パラメータ調整**: 重力、質量、粘性、密度などをGUIからリアルタイムに変更可能。

## 開発予定 (TODO)
- [ ] **水のレンダリング (Water Rendering)**: 粒子をメッシュ化、スクリーンスペース流体描画（Screen Space Fluid Rendering）を用いて実装予定。
