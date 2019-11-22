# lab3-1实验报告

小组成员 姓名 学号  
PB17000002 古宜民（队长）  
PB15081586 苏文治  
PB16001837 朱凡  

## 实验要求

根据C-minus语义，修改src/cminusc/cminus_builder.cpp，在助教提供的框架上正确实现一个C-minus语言的IR生成器，将前序实验中得到的语法树翻译到正确的LLVM IR。

## 实验难点

如何全局地传递表达式的值  
存在多个返回语句时的处理  
if或while语句中出现返回语句时的处理  
var可被存储或取值，需要区分  
GEP的使用  

## 实验设计

1. 如何设计全局变量  
BasicBlock* curr_block;  
BasicBlock* return_block:标注函数的返回语句所在BasicBlock，用于实现多个分支语句嵌套时的正确跳转  
Value* return_alloca:用于存储返回值  
Function* curr_func:用于创建BasicBlock时得到所在函数  
Value* expression:用于值的全局传递  
bool is_returned,is_returned_record:标注是否已出现返回语句，用于忽略返回语句后的语句  
int label_cnt:用于标签号管理  
var_op curr_op:用于确定当前对var作存储还是取值
2. 如何解决实验难点中的问题
3. ...


### 实验总结

此次实验有什么收获

### 实验反馈

对本次实验的建议（可选 不会评分）
