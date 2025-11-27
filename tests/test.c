// 测试高级循环控制：break 和 continue
int main() { 
  int sum = 0; // 累加结果
  
  // 循环 0 到 9
  for (int i = 0; i < 10; i = i + 1) { 
    if (i == 5) { 
      continue; // i为5时跳过本次循环，不累加
    } 
    if (i == 8) { 
      break; // i为8时直接终止整个循环
    } 
    sum = sum + i; // 累加: 0+1+2+3+4+6+7 = 23
  } 
  
  printf("Sum is: %d\n", sum); // 预期输出 23
  return 0; // 返回退出码 23
}