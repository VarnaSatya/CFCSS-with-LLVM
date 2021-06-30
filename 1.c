int max(int x, int y) {
  int k;
  if (x > y) k=x;
  else  k = y;
  return k;
}
void __ctt_error(int a , int b) {
  if (a != b) {
   fprintf(stderr, "Signatures are not matching ");
   exit(0);
  }
}