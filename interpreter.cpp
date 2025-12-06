// interpreter.cpp

#include "interpreter.h"
#include "disassembler.h" 
#include <iostream>
#include <iomanip>
#include <vector>
#include <stack>
#include <stdexcept>
#include <cstring>
#include <cmath> 
#include <algorithm> 
#include <sstream>

// Definir a macro ACC_STATIC se ela não estiver em classfile.h (é um flag de acesso)
#ifndef ACC_STATIC
#define ACC_STATIC 0x0008
#endif

// Tipos de array primitivos (adicionados para newarray)
#define T_BOOLEAN 4
#define T_CHAR    5
#define T_FLOAT   6
#define T_DOUBLE  7
#define T_BYTE    8
#define T_SHORT   9
#define T_INT     10 
#define T_LONG    11 


// =======================================================================
// 1. ESTRUTURAS AUXILIARES E GERENCIAMENTO DE HEAP
// =======================================================================

// Definição da Pilha de Frames e Heap
std::vector<Frame> jvm_stack;
std::vector<HeapObject> heap; 

// Construtor do Frame 
Frame::Frame(const MethodInfo& method, const ConstantPool& cp)
    : pc(0), class_constant_pool(&cp) {

    const CodeAttribute& code_attr = method.code_attribute;
    
    code = &code_attr.code;
    
    local_variables.resize(code_attr.max_locals, 0);
    operand_stack.reserve(code_attr.max_stack);
}

// Funções de Gerenciamento de Heap
jref allocate_heap_object(int type, size_t size, uint16_t class_index) {
    HeapObject obj;
    obj.type = type;
    obj.size = size; 
    obj.data.resize(size, 0); 
    obj.class_index = class_index;

    heap.push_back(std::move(obj));
    return (jref)heap.size() - 1; 
}


// =======================================================================
// 2. FUNÇÕES DE MANIPULAÇÃO DE DADOS (32 bits e 64 bits)
// =======================================================================

jword pop_jword(Frame& frame) {
    if (frame.operand_stack.empty()) {
        throw std::runtime_error("Erro: Pop em pilha de operandos vazia!");
    }
    jword value = frame.operand_stack.back();
    frame.operand_stack.pop_back();
    return value;
}

void push_jword(Frame& frame, jword value) {
    frame.operand_stack.push_back(value);
}

void push_jlong(Frame& frame, int64_t value) {
    uint64_t bits = (uint64_t)value; 
    push_jword(frame, (jword)(bits & 0xFFFFFFFF));
    push_jword(frame, (jword)(bits >> 32));
}

int64_t pop_jlong(Frame& frame) {
    uint64_t high_bytes = (uint64_t)pop_jword(frame);
    uint64_t low_bytes = (uint64_t)pop_jword(frame);
    uint64_t bits = (high_bytes << 32) | low_bytes;
    return (int64_t)bits;
}

void push_jdouble(Frame& frame, double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double)); 
    push_jlong(frame, (int64_t)bits);
}

double pop_jdouble(Frame& frame) {
    uint64_t bits = (uint64_t)pop_jlong(frame); 
    double d_val;
    std::memcpy(&d_val, &bits, sizeof(double));
    return d_val;
}


// Funções de leitura que avançam o PC (Usando o array 'code' do Frame)

uint8_t fetch_u1(Frame& frame) {
    if (frame.pc >= frame.code->size()) {
        throw std::runtime_error("Erro: PC fora dos limites do código.");
    }
    return frame.code->at(frame.pc++);
}

uint16_t fetch_u2(Frame& frame) {
    uint8_t b1 = fetch_u1(frame);
    uint8_t b2 = fetch_u1(frame);
    return (uint16_t)((b1 << 8) | b2);
}

int16_t fetch_s2(Frame& frame) {
    uint16_t u_val = fetch_u2(frame);
    int16_t s_val;
    std::memcpy(&s_val, &u_val, sizeof(int16_t));
    return s_val;
}

// =======================================================================
// 3. EXECUÇÃO PRINCIPAL (Loop Fetch-Decode-Execute)
// =======================================================================

void run_frame(Frame& frame) {
    
    while (frame.pc < frame.code->size()) {
        uint32_t offset = frame.pc;
        uint8_t opcode = fetch_u1(frame);
        
        // Output de Debug (Corretude)
        std::cout << "\t[PC: " << std::setw(4) << offset << "] Opcode: 0x" 
                  << std::hex << (int)opcode << std::dec;
        
        switch (opcode) {
            
            // --- CONSTANTES ---
            case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: 
            {
                int32_t val = (int32_t)opcode - 0x03; 
                push_jword(frame, (jword)val);
                std::cout << " -> iconst_" << val << std::endl;
                break;
            }
            case 0x10: // bipush 
            {
                int8_t val = (int8_t)fetch_u1(frame);
                push_jword(frame, (jword)val);
                std::cout << " -> bipush " << (int)val << std::endl;
                break;
            }
            case 0x11: { /* sipush */ // <--- INCLUÍDO
                int16_t short_val = fetch_s2(frame); 
                push_jword(frame, (jword)short_val);
                std::cout << " -> sipush " << short_val << std::endl; 
                break;
            }
            case 0x12: // ldc (Inteiros e Strings)
            {
                uint8_t index = fetch_u1(frame);
                const ConstantInfo& c = frame.class_constant_pool->at(index);
                
                if (c.tag == CONSTANT_Integer) {
                    push_jword(frame, c.bytes4);
                    std::cout << " -> ldc #" << (int)index << " (Int: " << (int32_t)c.bytes4 << ")" << std::endl;
                } else if (c.tag == CONSTANT_String) {
                    uint16_t utf8_index = c.index1;
                    const std::string& literal = frame.class_constant_pool->at(utf8_index).utf8_string;
                    
                    size_t string_size = literal.length(); 
                    jref string_ref = allocate_heap_object(3, string_size, 0); 
                    
                    for (size_t i = 0; i < string_size; ++i) {
                        heap[string_ref].data[i] = (jword)literal[i];
                    }

                    push_jword(frame, string_ref); 
                    std::cout << " -> ldc #" << (int)index << " (String Ref: " << string_ref << ", \"" << literal << "\")" << std::endl;
                } else {
                    throw std::runtime_error("LDC de tipo nao implementado: " + std::to_string(c.tag));
                }
                break;
            }
            
            // --- CONSTANTES (long/double - Categoria 2) ---
            case 0x09: case 0x0a: // lconst_0, lconst_1
            {
                int64_t val = (opcode == 0x09) ? 0L : 1L;
                push_jlong(frame, val);
                std::cout << " -> lconst_" << val << std::endl;
                break;
            }
            case 0x14: // ldc2_w
            {
                uint16_t index = fetch_u2(frame);
                const ConstantInfo& c = frame.class_constant_pool->at(index);

                if (c.tag == CONSTANT_Long) {
                    int64_t val = ((int64_t)c.high_bytes << 32) | c.low_bytes;
                    push_jlong(frame, val);
                    std::cout << " -> ldc2_w #" << index << " (Long: " << val << "l)" << std::endl;
                } else if (c.tag == CONSTANT_Double) {
                    uint64_t bits = ((uint64_t)c.high_bytes << 32) | c.low_bytes;
                    double val;
                    std::memcpy(&val, &bits, sizeof(double));
                    push_jdouble(frame, val);
                    std::cout << " -> ldc2_w #" << index << " (Double: " << val << "d)" << std::endl;
                } else {
                    throw std::runtime_error("LDC2_W de tipo invalido: " + std::to_string(c.tag));
                }
                break;
            }

            // --- VARIÁVEIS LOCAIS (Load - 32 bits / Referência) ---
            case 0x2a: case 0x2b: case 0x2c: case 0x2d: // aload_0 a aload_3 
            {
                uint8_t index = (uint8_t)opcode - 0x2a;
                jword ref = frame.local_variables.at(index);
                push_jword(frame, ref);
                std::cout << " -> aload_" << (int)index << " (Ref: " << ref << ")" << std::endl;
                break;
            }
            case 0x1a: case 0x1b: case 0x1c: case 0x1d: // iload_0 a iload_3
            {
                uint8_t index = (uint8_t)opcode - 0x1a;
                push_jword(frame, frame.local_variables.at(index));
                std::cout << " -> iload_" << (int)index << std::endl;
                break;
            }
            case 0x15: // iload 
            {
                uint8_t index = fetch_u1(frame);
                push_jword(frame, frame.local_variables.at(index));
                std::cout << " -> iload " << (int)index << std::endl;
                break;
            }

            // --- VARIÁVEIS LOCAIS (Store - 32 bits) ---
            case 0x36: { /* istore */ 
                uint8_t index = fetch_u1(frame);
                frame.local_variables.at(index) = pop_jword(frame);
                std::cout << " -> istore " << (int)index << std::endl;
                break;
            }
            case 0x3b: case 0x3c: case 0x3d: case 0x3e: // istore_0 a istore_3
            {
                uint8_t index = (uint8_t)opcode - 0x3b;
                frame.local_variables.at(index) = pop_jword(frame);
                std::cout << " -> istore_" << (int)index << std::endl;
                break;
            }
            
            // --- VARIÁVEIS LOCAIS (Store - Referência / astore_N) ---
            case 0x3a: { /* astore (índice variável) */
                uint8_t index = fetch_u1(frame);
                jword ref = pop_jword(frame);
                frame.local_variables.at(index) = ref;
                std::cout << " -> astore " << (int)index << " (Ref: " << ref << ")" << std::endl;
                break;
            }
            case 0x4b: case 0x4c: case 0x4d: case 0x4e: // astore_0 a astore_3
            {
                uint8_t index = (uint8_t)opcode - 0x4b;
                jword ref = pop_jword(frame);
                frame.local_variables.at(index) = ref;
                std::cout << " -> astore_" << (int)index << " (Ref: " << ref << ")" << std::endl;
                break;
            }

            // --- VARIÁVEIS LOCAIS (Load/Store - 64 bits) ---
            case 0x1e: case 0x1f: case 0x20: case 0x21: // lload_0 a lload_3
            {
                uint8_t index = (uint8_t)opcode - 0x1e;
                jword low = frame.local_variables.at(index);
                jword high = frame.local_variables.at(index + 1);
                
                push_jword(frame, low);
                push_jword(frame, high);
                std::cout << " -> lload_" << (int)index << std::endl;
                break;
            }
            case 0x37: { /* lstore (índice variável) */
                uint8_t index = fetch_u1(frame);
                int64_t val = pop_jlong(frame); 
                
                frame.local_variables.at(index) = (jword)(val & 0xFFFFFFFF);
                frame.local_variables.at(index + 1) = (jword)(val >> 32);
                std::cout << " -> lstore " << (int)index << std::endl;
                break;
            }
            case 0x3f: case 0x40: case 0x41: case 0x42: // lstore_0 a lstore_3
            {
                uint8_t index = (uint8_t)opcode - 0x3f;
                int64_t val = pop_jlong(frame); 
                
                frame.local_variables.at(index) = (jword)(val & 0xFFFFFFFF);
                frame.local_variables.at(index + 1) = (jword)(val >> 32);
                std::cout << " -> lstore_" << (int)index << std::endl;
                break;
            }

            // --- CRIAÇÃO E ACESSO A OBJETOS ---
            case 0xbb: // new
            {
                uint16_t class_index = fetch_u2(frame); 
                size_t fields_size = 4; // Tamanho simplificado
                
                jref new_ref = allocate_heap_object(0, fields_size, class_index); // Type 0: Objeto
                push_jword(frame, new_ref);
                
                std::string class_name = get_class_name(*frame.class_constant_pool, class_index);
                std::cout << " -> new #" << class_index << " (Ref: " << new_ref << ", Classe: " << class_name << ")" << std::endl;
                break;
            }
            
            case 0xbc: // newarray (Cria array de primitivos)
            {
                uint8_t atype = fetch_u1(frame); // Argumento de 1 byte: tipo do array
                
                int32_t count = (int32_t)pop_jword(frame); 
                
                if (count < 0) {
                    throw std::runtime_error("NegativeArraySizeException.");
                }
                
                jref array_ref = allocate_heap_object(atype, (size_t)count, 0); 
                push_jword(frame, array_ref);
                
                std::cout << " -> newarray (Type: " << (int)atype << ", Size: " << count << ", Ref: " << array_ref << ")" << std::endl;
                break;
            }

            case 0x2e: // iaload (Carrega INT de Array)
            {
                int32_t index = (int32_t)pop_jword(frame);
                jref array_ref = pop_jword(frame);
                
                if (array_ref == 0 || array_ref >= heap.size() || index < 0 || (size_t)index >= heap[array_ref].size) {
                    throw std::runtime_error("ArrayIndexOutOfBoundsException ou NullPointerException em iaload.");
                }
                
                jword value = heap[array_ref].data[index];
                push_jword(frame, value);
                
                std::cout << " -> iaload (Ref: " << array_ref << ", Index: " << index << ", Valor: " << (int32_t)value << ")" << std::endl;
                break;
            }

            case 0x4f: // iastore (Armazena INT em Array)
            {
                jword value = pop_jword(frame);
                int32_t index = (int32_t)pop_jword(frame);
                jref array_ref = pop_jword(frame);
                
                if (array_ref == 0 || array_ref >= heap.size() || index < 0 || (size_t)index >= heap[array_ref].size) {
                    throw std::runtime_error("ArrayIndexOutOfBoundsException ou NullPointerException em iastore.");
                }

                heap[array_ref].data[index] = value;
                
                std::cout << " -> iastore (Ref: " << array_ref << ", Index: " << index << ", Salvou: " << (int32_t)value << ")" << std::endl;
                break;
            }
            
            case 0xb2: // getstatic 
            {
                uint16_t field_index = fetch_u2(frame); 
                std::string field_ref_name = resolver_indice_cp_completo(*frame.class_constant_pool, field_index);
                push_jword(frame, 1); // Simulação: Push Ref: 1 para System.out
                
                std::cout << " -> getstatic #" << field_index << " (Carregou Ref: 1, Campo: " << field_ref_name << ")" << std::endl;
                break;
            }
            
            case 0xb4: // getfield 
            {
                uint16_t field_index = fetch_u2(frame); 
                jref object_ref = pop_jword(frame); 
                
                if (object_ref == 0 || object_ref >= heap.size()) {
                    throw std::runtime_error("Referencia nula ou invalida em getfield.");
                }
                
                jword field_value = heap[object_ref].data[1]; 
                
                push_jword(frame, field_value);
                std::cout << " -> getfield #" << field_index << " (Ref: " << object_ref << ", Valor: " << (int32_t)field_value << ")" << std::endl;
                break;
            }

            case 0xb5: // putfield 
            {
                uint16_t field_index = fetch_u2(frame);
                jword field_value = pop_jword(frame); 
                jref object_ref = pop_jword(frame); 
                
                if (object_ref == 0 || object_ref >= heap.size()) {
                    throw std::runtime_error("Referencia nula ou invalida em putfield.");
                }
                
                heap[object_ref].data[1] = field_value; 

                std::cout << " -> putfield #" << field_index << " (Ref: " << object_ref << ", Salvou: " << (int32_t)field_value << ")" << std::endl;
                break;
            }
            
            // --- UTILITÁRIOS DE PILHA (Mantidos) ---
            case 0x57: // pop
            {
                pop_jword(frame);
                std::cout << " -> pop" << std::endl;
                break;
            }
            case 0x59: // dup 
            {
                jword val = pop_jword(frame);
                push_jword(frame, val);
                push_jword(frame, val);
                std::cout << " -> dup" << std::endl;
                break;
            }
            case 0x58: // pop2
            {
                pop_jword(frame); 
                pop_jword(frame); 
                std::cout << " -> pop2" << std::endl;
                break;
            }

            // --- ARITMÉTICA ---
            case 0x60: // iadd
            {
                int32_t val2 = (int32_t)pop_jword(frame);
                int32_t val1 = (int32_t)pop_jword(frame);
                int32_t result = val1 + val2;
                push_jword(frame, (jword)result);
                std::cout << " -> iadd. Resultado: " << result << std::endl;
                break;
            }
            case 0x6c: // idiv (Divisão de inteiros)
            {
                int32_t val2 = (int32_t)pop_jword(frame); // Divisor
                int32_t val1 = (int32_t)pop_jword(frame); // Dividendo
                
                if (val2 == 0) {
                    std::cout << " -> idiv. ERRO: ArithmeticException (Divisao por zero)" << std::endl;
                    throw std::runtime_error("java/lang/ArithmeticException: / by zero");
                }

                int32_t result = val1 / val2;
                push_jword(frame, (jword)result);
                std::cout << " -> idiv. Resultado: " << result << std::endl;
                break;
            }
            case 0x61: // ladd
            {
                int64_t val2 = pop_jlong(frame);
                int64_t val1 = pop_jlong(frame);
                int64_t result = val1 + val2;
                push_jlong(frame, result);
                std::cout << " -> ladd. Resultado: " << result << "l" << std::endl;
                break;
            }

            // --- CONTROLE DE FLUXO (Mantidos) ---
            case 0xa7: // goto
            {
                int16_t offset_s16 = fetch_s2(frame); 
                frame.pc += (offset_s16 - 3); 
                std::cout << " -> goto " << (offset + offset_s16) << std::endl;
                break;
            }
            
            // --- CHAMADAS DE MÉTODO ---
            case 0xb7: // invokespecial 
            {
                uint16_t index = fetch_u2(frame); 
                std::string method_ref = resolver_indice_cp_completo(*frame.class_constant_pool, index);
                pop_jword(frame); // Consome a referência 'this'
                std::cout << " -> invokespecial #" << index << " (Chamada: " << method_ref << ") - *Referencia consumida*" << std::endl;
                break;
            }
            
            case 0xb6: // invokevirtual 
            {
                uint16_t method_index = fetch_u2(frame);
                std::string method_ref = resolver_indice_cp_completo(*frame.class_constant_pool, method_index);
                
                if (method_ref.find("java/io/PrintStream.println") != std::string::npos) {
                    
                    int32_t output_val = (int32_t)pop_jword(frame); 
                    pop_jword(frame); 

                    std::cout << " -> invokevirtual #" << method_index << " (Chamada: " << method_ref << ") - *Simulando*" << std::endl;
                    std::cout << "\n\t\t[OUTPUT SIMULADO] Valor impresso: " << output_val << std::endl;
                } else {
                    pop_jword(frame); // Consome a referência 'this' / objeto
                    std::cout << " -> invokevirtual #" << method_index << ". (Chamada: " << method_ref << ") - *Simulando*" << std::endl;
                }
                
                break;
            }
            
            case 0xb8: // invokestatic 
            {
                uint16_t index = fetch_u2(frame); 
                std::string method_ref = resolver_indice_cp_completo(*frame.class_constant_pool, index);
                
                if (method_ref.find("java/io/PrintStream.println") != std::string::npos) {
                    
                    jref arg_ref = pop_jword(frame); 
                    pop_jword(frame); 
                    
                    if (arg_ref > 0 && arg_ref < heap.size() && heap[arg_ref].type == 3) { 
                        std::cout << "\n\t\t[OUTPUT SIMULADO] String impressa: ";
                        for(jword c : heap[arg_ref].data) {
                            std::cout << (char)c;
                        }
                        std::cout << std::endl;
                    } else {
                         std::cout << "\n\t\t[OUTPUT SIMULADO] Valor impresso: " << (int32_t)arg_ref << std::endl;
                    }

                } else {
                    std::cout << " -> invokestatic #" << index << ". (Chamada: " << method_ref << ") - *Simulando*" << std::endl;
                }
                
                break;
            }
            
            // --- RETORNO ---
            case 0xb1: // return 
                std::cout << " -> return. Fim do Frame." << std::endl;
                return; 

            default:
                std::cerr << std::endl << "ERRO: Opcode nao implementado: 0x" << std::hex << (int)opcode << std::dec << std::endl;
                throw std::runtime_error("Instrucao nao suportada.");
        }
    }
}

// =======================================================================
// 4. FUNÇÃO DE COORDENAÇÃO (Chamada por jvm.cpp)
// =======================================================================

void executar_jvm(ClassFile& class_data) {
    // 1. Encontrar o método main
    MethodInfo* main_method = nullptr;
    for (auto& method : class_data.methods) {
        if (get_utf8(class_data.constant_pool, method.name_index) == "main" &&
            get_utf8(class_data.constant_pool, method.descriptor_index) == "([Ljava/lang/String;)V" &&
            (method.access_flags & ACC_STATIC)) {
            main_method = &method;
            break;
        }
    }

    if (!main_method || main_method->code_attribute.code_length == 0) {
        throw std::runtime_error("Nao foi encontrado o metodo 'main' executavel na classe.");
    }
    
    // --- CORREÇÃO: RESERVAR O SLOT 0 DO HEAP PARA NULL ---
    // Garante que todas as referências válidas serão > 0.
    if (heap.empty()) {
        heap.push_back({}); // Adiciona um objeto vazio para ser a Referência NULL (índice 0)
    }

    // 2. Inicializar o Frame de main 
    Frame main_frame(*main_method, class_data.constant_pool);
    
    // 3. Executar o Frame
    std::cout << "\n--- Iniciando a execucao de main ---" << std::endl;
    run_frame(main_frame);
    
    std::cout << "Execucao concluida. Pilha de execução vazia." << std::endl;
}