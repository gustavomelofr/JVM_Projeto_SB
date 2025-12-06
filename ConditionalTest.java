// ConditionalTest.java
public class ConditionalTest {
    public static void main(String[] args) {
        int a = 10;
        int b = 5;
        int result = 0;

        // --- Teste 1: Comparação Condicional (if_icmp) ---
        if (a > b) {
            result = 1;
        } else {
            result = 2;
        }

        // --- Teste 2: Loop (usa ifeq/ifne e goto) ---
        // O valor 3 é para garantir que o loop finalize rapidamente.
        int counter = 0; 
        while (counter < 3) {
            counter = counter + 1;
        }

        // Simulação de output final (usando instrução que você implementou)
        // Se a lógica estiver correta, result deve ser 1 e counter deve ser 3.
        // O valor final que queremos verificar é a soma de ambos.
        result = result + counter; // result deve ser 1 + 3 = 4

        // Aqui, precisaríamos de System.out.println(result) para verificar o output final.
    }
}