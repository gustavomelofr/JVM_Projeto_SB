// ExceptionTest.java
public class ExceptionTest {
    public static void main(String[] args) {
        int a = 10;
        int b = 0; // Isso causará a exceção

        try {
            int result = a / b; // FALHA AQUI (idiv)
            System.out.println(result); 
        } catch (ArithmeticException e) {
            // Este bloco será ignorado na simulação atual, 
            // mas a JVM deve detectar a falha antes de pular para cá.
            System.out.println(999); 
        }
    }
}