# lab4实验报告

组长 姓名 学号

小组成员 姓名 学号

## 实验要求

请按照自己的理解，写明本次实验需要干什么

## 报告内容

#### 1. RISC-V 机器代码的生成和运行

- LLVM 8.0.1适配RISC-V

  ......

- lab3-0 GCD样例 LLVM IR 生成 RISC-V源码的过程

  ......

- 安装 Spike模拟器并运行上述生成的RISC-V源码

  ......

#### 2. LLVM源码阅读与理解

- RegAllocFast.cpp 中的几个问题

  * *RegAllocFast* 函数的执行流程？

    答：
    
    RegAllocFast 是对 Machine IR 执行的，Machine IR 比 IR 更底层，可能同时存在虚拟寄存器与物理寄存器。
    虚拟寄存器是代表物理寄存器的符号，但不一定和物理寄存器一一对应，如平时写的.ll文件中，%0 %1之类的即为虚拟寄存器。物理寄存器是与机器相关的实际存在的寄存器。RegAllocFast主要任务是，把还存在的虚拟寄存器与物理寄存器对应起来，完成寄存器的分配。
    
    整个流程为：

    - createFastRegisterAllocator 创建 RegAllocFast 实例
    - 对每个Machine Function执行runOnMachineFunction，runOnMachineFunction流程为：
        - 初始化虚拟寄存器与物理寄存器对应的的map
        - 对每个Machine BasicBlock执行allocateBasicBlock，allocateBasicBlock流程为：
            - 把live-in寄存器标记为regReserved。live-in寄存器是指在执行这条指令之前，这个寄存器是活的。被标记为regReserved后，该寄存器将被保留不会被分配。
            - 对每条指令，执行allocateInstruction（内有四次扫描，执行流程见下一题的回答）
            - spill当下还与虚拟寄存器对应的物理寄存器，从而把物理寄存器腾出来，为下一个BasicBlock的使用做准备。
            - 从BasicBlock中移除可以合并的copy指令
        - 已经将所有虚拟寄存器替换为物理寄存器了，从RegInfo记录信息中移除所有虚拟寄存器

  * *allocateInstruction* 函数有几次扫描过程以及每一次扫描的功能？

    答：
    
    - 第一次扫描：
      - 确定指令属性，初始化
      - 对内联汇编特殊处理
    - 第二次扫描：
      - 对对应于Virtual Register的操作数进行虚拟寄存器分配，并分配该虚拟寄存器对应的物理寄存器
      - 
    - 第三次扫描：

  * *calcSpillCost* 函数的执行流程？

    答：*calcSpillCost*作用于Physical Register上，用于计算如果这个物理寄存器被换出到内存需要付出的代价。

    - 首先判断寄存器是否在本条指令中被使用，如果被使用则不可spill，返回impossible；

    如果不在当前指令被使用，函数分为两种情况，即这个物理寄存器是否处于disabled状态。

    - 如果没有被disabled，即处于活跃状态，则直接分析该物理寄存器的状态：

      - 可用(regFree)代价为0；

      - 被保留不可用(regReserved)返回不可能；

      - 被分给了虚拟寄存器则再判断其是否被修改过需要写回内存(Dirty)，赋予其代价spillDirty(100)或spillClean(50)。

        因为如果VirtReg不Dirty，spill带来的代价为再次使用时Load一次访存；而Dirty需要进行写回内存操作，代价为Store一次访存，用时Load再一次访存，其代价为不Dirty的二倍，所以spillDirty为spillClean的二倍。

    - 如果被disabled，说明该寄存器被其他变量占用（可能因为alias被分割并分配给了很多其他变量），而所有这些变量都要被spill。此时需要遍历所有的alias，对每个alias作类似第一种情况的分析。
      - alias为可用，则代价稍微增加1(++Cost)，这保证即使是free的，alias也是越少越好；
      - alias被保留不可用，则整个寄存器也不可用，返回Impossible；
      - alias被分给虚拟寄存器，则同上判断是否Dirty，带来一个50或100较大的Cost。

  * *hasTiedOps*，*hasPartialRedefs，hasEarlyClobbers* 变量的作用？

    答：各个变量的含义：
    
    - *hasTiedOps*：两个操作数为Tied意为这两个操作数的限制为必须对应于同一个寄存器。如果一条指令中有操作数包含了这种限制，则hasTiedOps为真。
    - *hasPartialRedefs*：Partial Redefination指一个寄存器的一部分(subregister)被重新定义。
    - *hasEarlyClobbers*：earlyclobber的操作数表示这个操作数在指令执行结束前就被（根据输入操作数）写覆盖了(Ref: [gcc](https://gcc.gnu.org/onlinedocs/gcc/Modifiers.html#Modifiers))。因此这一操作数不能被存储在这条指令会读取的寄存器中，也不能存储在

- 书上所讲的算法与LLVM源码中的实现之间的不同点

  ......



## 组内讨论内容

......

## 实验总结

此次实验有什么收获

## 实验反馈

对本次实验的建议（可选 不会评分）