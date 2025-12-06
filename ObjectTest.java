// ObjectTest.java
public class ObjectTest {
    // Definimos uma variável estática (simulada) para o System.out
    // Na JVM real, System.out é um Field estático.
    // Para este teste, focamos no método main.

    public static void main(String[] args) {
        // Teste 1: String Literal (usa ldc de String)
        String nome = "JVM Project"; 
        
        // Teste 2: Criação de Objeto Simples (usa new e invokespecial)
        Object obj = new Object(); // Usa new e invokespecial <init>
        
        // Teste 3: Simulação de Atribuição de Campo (putfield)
        // Como 'Object' não tem campos, vamos simular uma atribuição
        // usando a própria referência como um valor.
        int a = 42; 
        
        // A JVM compilará algo como: putfield obj.campo = a;
        // Se a atribuição for bem-sucedida, o campo no Heap deve ser 42.

        // Teste 4: I/O Simulado (usa getstatic/invokevirtual/invokestatic)
        // Isso forçará o invokestatic no seu interpretador
        // System.out.println("Hello"); // Se usarmos String, precisamos do invokevirtual
        
        // Vamos usar o System.out.println(int) que é mais simples
        System.out.println(a); 
    }
}