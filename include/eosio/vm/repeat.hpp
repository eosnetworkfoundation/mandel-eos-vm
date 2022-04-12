#pragma once

#define EOS_VM_REPEAT_0(m)
#define EOS_VM_REPEAT_1(m) m(0)
#define EOS_VM_REPEAT_2(m) m(0) m(1)
#define EOS_VM_REPEAT_3(m) m(0) m(1) m(2)
#define EOS_VM_REPEAT_4(m) m(0) m(1) m(2) m(3)
#define EOS_VM_REPEAT_5(m) EOS_VM_REPEAT_1(m) m(1) m(2) m(3) m(4)
#define EOS_VM_REPEAT_6(m) EOS_VM_REPEAT_2(m) m(2) m(3) m(4) m(5)
#define EOS_VM_REPEAT_7(m) EOS_VM_REPEAT_3(m) m(3) m(4) m(5) m(6)
#define EOS_VM_REPEAT_8(m) EOS_VM_REPEAT_4(m) m(4) m(5) m(6) m(7)
#define EOS_VM_REPEAT_9(m) EOS_VM_REPEAT_5(m) m(5) m(6) m(7) m(8)
#define EOS_VM_REPEAT_10(m) EOS_VM_REPEAT_6(m) m(6) m(7) m(8) m(9)
#define EOS_VM_REPEAT_11(m) EOS_VM_REPEAT_7(m) m(7) m(8) m(9) m(10)
#define EOS_VM_REPEAT_12(m) EOS_VM_REPEAT_8(m) m(8) m(9) m(10) m(11)
#define EOS_VM_REPEAT_13(m) EOS_VM_REPEAT_9(m) m(9) m(10) m(11) m(12)
#define EOS_VM_REPEAT_14(m) EOS_VM_REPEAT_10(m) m(10) m(11) m(12) m(13)
#define EOS_VM_REPEAT_15(m) EOS_VM_REPEAT_11(m) m(11) m(12) m(13) m(14)
#define EOS_VM_REPEAT_16(m) EOS_VM_REPEAT_12(m) m(12) m(13) m(14) m(15)

#define EOS_VM_ENUM_0(t)
#define EOS_VM_ENUM_1(t) t ## 0
#define EOS_VM_ENUM_2(t) t ## 0, t ## 1
#define EOS_VM_ENUM_3(t) t ## 0, t ## 1, t ## 2
#define EOS_VM_ENUM_4(t) t ## 0, t ## 1, t ## 2, t ## 3
#define EOS_VM_ENUM_5(t) EOS_VM_ENUM_1(t), t ## 1, t ## 2, t ## 3, t ## 4
#define EOS_VM_ENUM_6(t) EOS_VM_ENUM_2(t), t ## 2, t ## 3, t ## 4, t ## 5
#define EOS_VM_ENUM_7(t) EOS_VM_ENUM_3(t), t ## 3, t ## 4, t ## 5, t ## 6
#define EOS_VM_ENUM_8(t) EOS_VM_ENUM_4(t), t ## 4, t ## 5, t ## 6, t ## 7
#define EOS_VM_ENUM_9(t) EOS_VM_ENUM_5(t), t ## 5, t ## 6, t ## 7, t ## 8
#define EOS_VM_ENUM_10(t) EOS_VM_ENUM_6(t), t ## 6, t ## 7, t ## 8, t ## 9
#define EOS_VM_ENUM_11(t) EOS_VM_ENUM_7(t), t ## 7, t ## 8, t ## 9, t ## 10
#define EOS_VM_ENUM_12(t) EOS_VM_ENUM_8(t), t ## 8, t ## 9, t ## 10, t ## 11
#define EOS_VM_ENUM_13(t) EOS_VM_ENUM_9(t), t ## 9, t ## 10, t ## 11, t ## 12
#define EOS_VM_ENUM_14(t) EOS_VM_ENUM_10(t), t ## 10, t ## 11, t ## 12, t ## 13
#define EOS_VM_ENUM_15(t) EOS_VM_ENUM_11(t), t ## 11, t ## 12, t ## 13, t ## 14
#define EOS_VM_ENUM_16(t) EOS_VM_ENUM_12(t), t ## 12, t ## 13, t ## 14, t ## 15
