# cloth_simulation_demo
用C++和opengl完成了一下 https://mp.weixin.qq.com/s/x6qX_SXCUdlTmGulx6okAg 的hard挑战^_^

一个visual studio工程，两个C++文件cloth.h和main.cpp，四个着色器文件，总计约800行代码。在个人的机器上，release模式执行可以达到60帧，而用taichi指定ti.init(arch=ti.cpu)，运行大约36帧，指定ti.init(arch=ti.cuda)运行可以达到117帧左右，指定ti.init(arch=ti.vulkan)甚至可以达到215帧。本仓库根目录下的cloth_simulation.py即为相应的taichi实现。

几处修改：

1、可以加好几个球，只要在main.cpp里改一下ball_number就行。默认是5个球。在shading模型中增加距离项，使得离光源更远的小球看上去更暗。

2、添加了布料和球的摩擦，但其实只是简单的将速度乘以0.99>_<。

3、把重力和弹簧的弹力放到一个循环里面计算，而不是一开始先单独用重力更新一下速度。

4、把弹簧的劲度系数调小了一些。

5、其余参数与taichi原本的实现一致。然后照着learnopengl抄了一个shading模型，只是小球看上去跟taichi的ggui画出来的不太一样，可能是blinn-phong光照模型的一些参数引起的。

碰撞与布料的自相交之类的就太难了（特别是球多又小的情况下，自相交问题非常严重），果断放弃

# 运行环境与个人配置
visual studio 2019，同时需要在visual studio中自行配置opengl3.3(glfw3 & glad & glm0.9.9)，Eigen 3.3.9, 以及onetbb。可能还需要将visual studio设置为C++17版本。

配置情况：Intel i7-11800h（8核16线程）， RTX 3060 Laptop GPU（显存6GB）， 16GB内存（8GB*2）双通道， windows10

# 运行结果
![image](https://github.com/tlcui/cloth_simulation_demo/blob/master/results_new.gif)

# 其他
借助调的一大堆库，这个C++的实现帧数能战胜ti.cpu。我的程序里的并行计算几乎全是由tbb::parallel_for()完成的，同时CPU负载也明显更高，运行一段时间后笔记本风扇就直接起飞（温度93°C），而ti.cpu执行时CPU温度在83°C至88°C之间。（尽管看任务管理器这两者的CPU占用都是100%）

