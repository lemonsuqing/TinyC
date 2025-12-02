struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    printf("p.x + p.y = %d\n", p.x + p.y);
    return 0; // 30
}