// ArrayTest.java
public class ArrayTest {
    public static void main(String[] args) {
        // 1. Cria um array de 4 inteiros (usa iconst_4, newarray)
        int[] numbers = new int[4]; 

        // 2. Armazena um valor no índice 2 (usa iconst_2, iconst_42, iastore)
        numbers[2] = 42; 

        // 3. Recupera o valor do índice 2 (usa iconst_2, iaload)
        int retrievedValue = numbers[2]; 

        // 4. Imprime o valor (usa getstatic, iload, invokevirtual)
        System.out.println(retrievedValue); // Deve imprimir 42
    }
}