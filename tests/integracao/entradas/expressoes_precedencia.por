programa {
    inteiro a = 2 + 3 * 4;
    inteiro b = (2 + 3) * 4;
    inteiro c = 1 << 3 | 2 & 3 ^ 4;
    logico d = a > b && b <= c || !(a == c);
    inteiro v[5];
    v[0] = a % 3;
    v[1] = -a + +b;
    v[2] = ~c;
    a >>= 1;
    b *= 2;
}
