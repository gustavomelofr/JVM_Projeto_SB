// LongTest.java
public class LongTest {
    public static void main(String[] args) {
        long x = 10000000000L; // ldc2_w, lstore_0
        long y = 5L;          // lconst_5 (ou ldc2_w), lstore_2 (long usa 2 slots, então x está em 0 e 1)
        long z = x + y;       // lload_0, lload_2, ladd, lstore_4
        // System.out.println(z);
    }
}