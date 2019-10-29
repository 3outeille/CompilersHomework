int gcd (int u, int v) { /* calculate the gcd of u and v */
  if (v == 0) return u;
  else return gcd(v, u - u / v * v); /* v,u-u/v*v is equals to u mod v*/
}
int main() {
  int x; int y; int temp;
  x = 72;
  y = 18;
  if (x<y) {
    temp = x;
    x = y;
    y = temp;
  }
  return gcd(x,y);
}

