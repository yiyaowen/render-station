# RenderStation
现在开始使用渲染作坊&lt;DX12>，一起创造奇妙的动画与游戏。Create fantastic animation and game with Render Station&lt;DX12>.

## 演示程序渲染结果

### 纯色立方体

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/root_descriptor_table.png)

### 基础贴图

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/import_texture.png)

### 透明混合（水+雾+铁丝网）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/ps_blend.png)

### 简单的平面镜像

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/basic_stencil_tech.png)

### 使用混合技术实现简单平面阴影（渲染出错！）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/double_blend_error.png)

### 使用模板缓冲区修正二次混合错误

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/use_stencil_avoid_double_blend.png)

### 绘制平面阴影的镜像

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/planar_shadow_mirror.png)

### 深度复杂度可视化

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/depth_complexity_with_blend_add.png)

### 公告牌技术（未使用AlphaToCoverage技术，透明层边缘锯齿严重）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/noMSAA-noAlphaToCoverageEnable-jagged.png)

### 使用AlphaToCoverage技术平滑透明层边缘

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/doMSAA-doAlphaToCoverage-anti_alias_smooth.png)

### 拓展圆环为空心圆柱（GS）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/generate_side_cylinder_from_cap_ring_with_gs.png)

### 顶点法线（红色）、面（三角）法线（绿色）可视化

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/red_ver_normal_green_tri_normal_visible.png)

### 高斯模糊后期处理（模糊半径 = 5 pixels，模糊度 = 256.0f，模糊次数 = 12）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/gaussian_blur_series/blur_king/gaussian_blur_radius=5_grade=256_count=12.png)

### Gaussian Blur VS. Bilateral Blur

高斯模糊 对比 双边模糊，双边模糊会保留更多的边缘细节，但是运算量比高斯模糊大得多！

原图：

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/origin.png)

高斯模糊：

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/gaussian-blur-r=5pixels-g=256.0f-c=1.png)

双边模糊：

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/bilateral-blur-r=5pixels-dg=256.0f-gg=0.1f-c=1.png)

### 水面波浪模拟（非实时力学模拟，基于简单的波浪方程）

![](https://gitee.com/yiyaowen/render-station-archived/raw/demo-images/wave-simulation.png)

=== 我也是有底线的 ===
