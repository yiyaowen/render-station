# RenderStation
现在开始使用渲染作坊&lt;DX12>，一起创造奇妙的动画与游戏。Create fantastic animation and game with Render Station&lt;DX12>.

## 项目构建注意事项

建议使用 Visual Studio 构建，需要安装 C/C++ 环境。首先下载 Release 中的 rs-models-textures.zip 资源包，然后解压并将其中的 textures 和 models 两个文件夹复制到项目根目录下（也即 RSC.sln 同级路径下），最后使用图形界面或者命令行 MSBuild 启动 solution 或者 vcxproj 配置文件。

## 演示程序渲染结果

### 纯色立方体

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/root_descriptor_table.png)

### 基础贴图

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/import_texture.png)

### 透明混合（水+雾+铁丝网）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/ps_blend.png)

### 简单的平面镜像

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/basic_stencil_tech.png)

### 使用混合技术实现简单平面阴影（渲染出错！）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/double_blend_error.png)

### 使用模板缓冲区修正二次混合错误

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/use_stencil_avoid_double_blend.png)

### 绘制平面阴影的镜像

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/planar_shadow_mirror.png)

### 深度复杂度可视化

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/depth_complexity_with_blend_add.png)

### 公告牌技术（未使用AlphaToCoverage技术，透明层边缘锯齿严重）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/noMSAA-noAlphaToCoverageEnable-jagged.png)

### 使用AlphaToCoverage技术平滑透明层边缘

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/doMSAA-doAlphaToCoverage-anti_alias_smooth.png)

### 拓展圆环为空心圆柱（GS）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/generate_side_cylinder_from_cap_ring_with_gs.png)

### 顶点法线（红色）、面（三角）法线（绿色）可视化

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/red_ver_normal_green_tri_normal_visible.png)

### 高斯模糊后期处理（模糊半径 = 5 pixels，模糊度 = 256.0f，模糊次数 = 12）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/gaussian_blur_series/blur_king/gaussian_blur_radius=5_grade=256_count=12.png)

### Gaussian Blur VS. Bilateral Blur

高斯模糊 对比 双边模糊，双边模糊会保留更多的边缘细节，但是运算量比高斯模糊大得多！

原图：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/origin.png)

高斯模糊：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/gaussian-blur-r=5pixels-g=256.0f-c=1.png)

双边模糊：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/bilateral-blur-r=5pixels-dg=256.0f-gg=0.1f-c=1.png)

### 水面波浪模拟（非实时力学模拟，基于简单的波浪方程）

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/wave-simulation.png)

### 512x512水面网格波浪模拟，CPU-General-Compute VS. GPU-CS-Optimization

计算着色器加速如德芙般丝滑，使用CPU直接计算三帧电竞！

CPU直接计算，10帧以下：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/wave_simulation_with_CPU_general_compute.png)

GPU计算着色器优化，稳定40帧以上：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/wave_simulation_with_GPU_CS_optimization.png)

### 表面细分着色器示例，从上往下距离依次拉近，细分程度越来越高

超远，近乎平面，无任何细节：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/hill-tessellation-flat.png)

非常远，几乎无细节：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/hill-tessellation-very-far.png)

远距离，开始出现细节：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/hill-tessellation-far.png)

中等距离，细节进一步增多：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/hill-tessellation-middle.png)

近距离，最高细节：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/hill-tessellation-near.png)

使用动态资源索引技术构建的场景：

![](https://raw.githubusercontent.com/yiyaowen/render-station-demo-image/main/a_test_scene_with_dynamic_resource_index.png)

=== 我也是有底线的 ===
