# cloth_simulation_demo
用C++和opengl完成了一下 https://mp.weixin.qq.com/s/x6qX_SXCUdlTmGulx6okAg的hard挑战^_^
一个visual studio工程，两个文件cloth.h和main.cpp，总计约700行C++代码（其中有数十行的shader），在本机release模式执行可以达到60帧（不知道是不是我算帧数的方法不对。。。见main.cpp的259至264行），而用taichi指定ti.init(arch=ti.cpu)
运行大约36帧，指定ti.init(arch=ti.cuda)运行可以达到117帧左右。

几处修改：

1、可以加好几个球，只要在main.cpp里改一下ball_number就行。默认是5个球。

2、添加了布料和球的摩擦，但其实只是简单的将速度乘以0.99>_<。

3、把重力和弹簧的弹力放到一个循环里面算了，而不是一开始先单独用重力更新一下速度，不知道这样合不合理。

4、把弹簧的劲度系数调小了一些。

5、其余参数与taichi原本的实现一致，然后照球learnopengl抄了一个光照模型，只是小球看上去跟taichi的ggui画出来的不太一样。。

碰撞与布料的自相交之类的就太难了（特别是球多又小的情况下，自相交问题非常严重），果断放弃

# 运行环境与个人配置
visual studio 2019，同时需要在visual studio中自行配置opengl3.3(glfw3 & glad & glm0.9.9)，Eigen 3.3.9, 以及onetbb。

配置情况：Intel i7-11800h（8核16线程）， RTX 3060 Laptop GPU（显存6GB）， 16GB内存（8GB*2）双通道， windows10

# 运行结果
![image](https://github.com/tlcui/cloth_simulation_demo/blob/master/results.gif)

# 其他
多亏调的一大堆库，这个C++的实现帧数能战胜ti.cpu。我的程序里的并行计算几乎全是由tbb::parallel_for()完成的，同时CPU负载也明显更高，因为运行一段时间后笔记本风扇就直接起飞（温度93°C），而ti.cpu执行时CPU温度在83°C至88°C之间。（尽管看任务管理器这两者的CPU占用都是100%）

