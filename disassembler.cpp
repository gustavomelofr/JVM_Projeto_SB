// disassembler.cpp

#include "disassembler.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cmath> 
#include <algorithm>

// =======================================================================
// 1. FUNÇÕES DE EXIBIÇÃO E RESOLUÇÃO (Implementações)
// =======================================================================

// Implementação completa para resolver e descrever o índice do CP
std::string resolver_indice_cp_completo(const ConstantPool& pool, uint16_t index) {
    if (index == 0 || index >= pool.size()) return "[Indice invalido]";

    try {
        const ConstantInfo& c = pool.at(index);
        std::string s = "";
        
        // Resolve referências compostas (Fieldref, Methodref, InterfaceMethodref)
        if (c.tag >= CONSTANT_Fieldref && c.tag <= CONSTANT_InterfaceMethodref) {
            std::string class_name = get_class_name(pool, c.index1);
            const ConstantInfo& nat = pool.at(c.index2); // NameAndType
            std::string name = get_utf8(pool, nat.index1);
            std::string desc = get_utf8(pool, nat.index2);
            s += class_name + ".\"" + name + "\":" + desc;
        } else if (c.tag == CONSTANT_Class) {
            s += "Class " + get_class_name(pool, index);
        } else if (c.tag == CONSTANT_String) {
            s += "String \"" + get_utf8(pool, c.index1) + "\"";
        } else if (c.tag == CONSTANT_NameAndType) {
            s += "NameAndType \"" + get_utf8(pool, c.index1) + "\":" + get_utf8(pool, c.index2);
        } else if (c.tag == CONSTANT_Integer) {
            s += "Int " + std::to_string((int32_t)c.bytes4);
        } else if (c.tag == CONSTANT_Float) {
            float f_val;
            std::memcpy(&f_val, &c.bytes4, sizeof(float));
            s += "Float " + std::to_string(f_val) + "f";
        } else if (c.tag == CONSTANT_Long) {
            int64_t l_val = ((int64_t)c.high_bytes << 32) | c.low_bytes;
            s += "Long " + std::to_string(l_val) + "l";
        } else if (c.tag == CONSTANT_Double) {
            uint64_t bits = ((uint64_t)c.high_bytes << 32) | c.low_bytes;
            double d_val;
            std::memcpy(&d_val, &bits, sizeof(double));
            s += "Double " + std::to_string(d_val) + "d";
        } else {
            s += "[Tipo " + std::to_string(c.tag) + " nao resolvido]";
        }
        return s;
    } catch (const std::exception& e) {
        return "[Erro ao resolver indice]";
    }
}


void exibir_constant_pool(const ConstantPool& pool) {
    std::cout << "\n--- Constant Pool ---" << std::endl;
    for (size_t i = 1; i < pool.size(); i++) {
        std::cout << std::setw(4) << std::right << "#" << i << " = ";
        const ConstantInfo& c = pool[i];
        
        std::string type_str;
        std::string value_str;

        switch (c.tag) {
            case CONSTANT_Utf8: type_str = "Utf8"; value_str = "\"" + c.utf8_string + "\""; break;
            case CONSTANT_Class: type_str = "Class"; value_str = "#" + std::to_string(c.index1) + "\t\t// " + get_utf8(pool, c.index1); break;
            case CONSTANT_String: type_str = "String"; value_str = "#" + std::to_string(c.index1) + "\t\t// " + get_utf8(pool, c.index1); break;
            case CONSTANT_Fieldref: type_str = "Fieldref"; value_str = "#" + std::to_string(c.index1) + ".#" + std::to_string(c.index2) + "\t// " + resolver_indice_cp_completo(pool, i); break;
            case CONSTANT_Methodref: type_str = "Methodref"; value_str = "#" + std::to_string(c.index1) + ".#" + std::to_string(c.index2) + "\t// " + resolver_indice_cp_completo(pool, i); break;
            case CONSTANT_InterfaceMethodref: type_str = "InterfaceMethodref"; value_str = "#" + std::to_string(c.index1) + ".#" + std::to_string(c.index2) + "\t// " + resolver_indice_cp_completo(pool, i); break;
            case CONSTANT_NameAndType: type_str = "NameAndType"; value_str = "#" + std::to_string(c.index1) + ".#" + std::to_string(c.index2) + "\t// " + get_utf8(pool, c.index1) + ":" + get_utf8(pool, c.index2); break;
            case CONSTANT_Integer: type_str = "Integer"; value_str = std::to_string((int32_t)c.bytes4); break;
            case CONSTANT_Float: { float f_val; std::memcpy(&f_val, &c.bytes4, sizeof(float)); type_str = "Float"; value_str = std::to_string(f_val) + "f"; break; }
            case CONSTANT_Long: { type_str = "Long"; value_str = resolver_indice_cp_completo(pool, i); i++; break; } // Ocupa 2 slots
            case CONSTANT_Double: { type_str = "Double"; value_str = resolver_indice_cp_completo(pool, i); i++; break; } // Ocupa 2 slots
            case 0: type_str = "(Slot vazio)"; value_str = ""; break;
            default: type_str = "Tag Desconhecida"; value_str = std::to_string(c.tag); break;
        }
        std::cout << std::setw(20) << std::left << type_str << value_str << std::endl;
    }
}

void exibir_class_info(const ClassFile& class_data) {
    std::cout << "\n--- Informacoes da Classe ---" << std::endl;
    std::cout << "Magic: 0x" << std::hex << class_data.magic << std::dec << std::endl;
    std::cout << "Versao: " << class_data.major_version << "." << class_data.minor_version << std::endl;
    std::cout << "flags: 0x" << std::hex << class_data.access_flags << std::dec << std::endl;
    std::cout << "this_class: #" << class_data.this_class_idx << " \t\t// " << get_class_name(class_data.constant_pool, class_data.this_class_idx) << std::endl;
    
    if (class_data.super_class_idx > 0) {
        std::cout << "super_class: #" << class_data.super_class_idx << " \t// " << get_class_name(class_data.constant_pool, class_data.super_class_idx) << std::endl;
    } else {
        std::cout << "super_class: #" << class_data.super_class_idx << " \t// (java/lang/Object)" << std::endl;
    }

    std::cout << "interfaces_count: " << class_data.interfaces.size() << std::endl;
    for (size_t i = 0; i < class_data.interfaces.size(); i++) {
        uint16_t interface_idx = class_data.interfaces[i];
        std::cout << "\tInterface #" << interface_idx << " \t// " << get_class_name(class_data.constant_pool, interface_idx) << std::endl;
    }
}

void exibir_fields(const ClassFile& class_data) {
    std::cout << "\n--- Fields (Contagem: " << class_data.fields.size() << ") ---" << std::endl;
    for (size_t i = 0; i < class_data.fields.size(); i++) {
        const FieldInfo& f = class_data.fields[i];
        std::cout << "Field #" << i << ":" << std::endl;
        std::cout << "\tNome: " << get_utf8(class_data.constant_pool, f.name_index) << std::endl;
        std::cout << "\tDescritor: " << get_utf8(class_data.constant_pool, f.descriptor_index) << std::endl;
        std::cout << "\tflags: 0x" << std::hex << f.access_flags << std::dec << std::endl;
        std::cout << "\tattributes_count: " << f.attributes_count << std::endl;
    }
}

void exibir_methods(const ClassFile& class_data) {
    std::cout << "\n--- Methods (Contagem: " << class_data.methods.size() << ") ---" << std::endl;
    for (size_t i = 0; i < class_data.methods.size(); i++) {
        const MethodInfo& m = class_data.methods[i];
        std::cout << "\nMethod #" << i << ":" << std::endl;
        std::cout << "\tNome: " << get_utf8(class_data.constant_pool, m.name_index) << std::endl;
        std::cout << "\tDescritor: " << get_utf8(class_data.constant_pool, m.descriptor_index) << std::endl;
        std::cout << "\tflags: 0x" << std::hex << m.access_flags << std::dec << std::endl;
        
        if (m.code_attribute.code_length > 0) {
            std::cout << "\t\t--- Code Attribute ---" << std::endl;
            std::cout << "\t\tmax_stack: " << m.code_attribute.max_stack << std::endl;
            std::cout << "\t\tmax_locals: " << m.code_attribute.max_locals << std::endl;
            std::cout << "\t\tcode_length: " << m.code_attribute.code_length << std::endl;
            std::cout << "\t\tBytecode:" << std::endl;
            
            desmontar_bytecode(m.code_attribute.code, class_data.constant_pool);
        } else {
            std::cout << "\t(Metodo nao possui bytecode executavel)" << std::endl;
        }
    }
}

// =======================================================================
// 2. FUNÇÃO DE DESMONTAGEM (DISASSEMBLER)
// =======================================================================

void desmontar_bytecode(const std::vector<uint8_t>& code, const ConstantPool& pool) {
    size_t pc = 0;
    while (pc < code.size()) {
        size_t offset = pc;
        uint8_t opcode = code[pc++];

        std::cout << "\t\t\t\t" << std::setw(4) << offset << ": ";

        // Ponteiros de leitura (lambdas) que avançam o pc do disassembler
        auto get_u1 = [&]() { return code[pc++]; };
        auto get_u2 = [&]() { 
            uint8_t b1 = code[pc++];
            uint8_t b2 = code[pc++];
            return (uint16_t)((b1 << 8) | b2);
        };
        auto get_s2 = [&]() { 
            int16_t s_val; 
            uint16_t u_val = get_u2();
            std::memcpy(&s_val, &u_val, sizeof(int16_t));
            return s_val;
        };
        auto get_s4 = [&]() { 
            int32_t s_val; 
            uint32_t u_val;
            u_val = (uint32_t)(get_u1() << 24 | get_u1() << 16 | get_u1() << 8 | get_u1());
            std::memcpy(&s_val, &u_val, sizeof(int32_t));
            return s_val;
        };


        switch (opcode) {
            // --- Opcodes sem argumentos ---
            case 0x00: std::cout << "nop" << std::endl; break;
            case 0x01: std::cout << "aconst_null" << std::endl; break;
            case 0x02: std::cout << "iconst_m1" << std::endl; break;
            case 0x03: std::cout << "iconst_0" << std::endl; break;
            case 0x04: std::cout << "iconst_1" << std::endl; break;
            case 0x05: std::cout << "iconst_2" << std::endl; break;
            case 0x06: std::cout << "iconst_3" << std::endl; break;
            case 0x07: std::cout << "iconst_4" << std::endl; break;
            case 0x08: std::cout << "iconst_5" << std::endl; break;
            case 0x09: std::cout << "lconst_0" << std::endl; break;
            case 0x0a: std::cout << "lconst_1" << std::endl; break;
            case 0x57: std::cout << "pop" << std::endl; break; 
            case 0x58: std::cout << "pop2" << std::endl; break;
            case 0x60: std::cout << "iadd" << std::endl; break;
            case 0x6c: std::cout << "idiv" << std::endl; break; 
            case 0xb1: std::cout << "return" << std::endl; break;


            // --- LOAD/STORE RÁPIDO (aload_N, iload_N, istore_N) ---
            case 0x2a: std::cout << "aload_0" << std::endl; break;
            case 0x2b: std::cout << "aload_1" << std::endl; break;
            case 0x2c: std::cout << "aload_2" << std::endl; break;
            case 0x2d: std::cout << "aload_3" << std::endl; break;
            case 0x1a: std::cout << "iload_0" << std::endl; break;
            case 0x1b: std::cout << "iload_1" << std::endl; break;
            case 0x1c: std::cout << "iload_2" << std::endl; break; 
            case 0x1d: std::cout << "iload_3" << std::endl; break; 
            case 0x1e: std::cout << "lload_0" << std::endl; break;
            case 0x3b: std::cout << "istore_0" << std::endl; break;
            case 0x3c: std::cout << "istore_1" << std::endl; break; 
            case 0x3d: std::cout << "istore_2" << std::endl; break; 
            case 0x3e: std::cout << "istore_3" << std::endl; break; 
            case 0x3f: std::cout << "lstore_0" << std::endl; break;
            case 0x4b: std::cout << "astore_0" << std::endl; break;
            case 0x4c: std::cout << "astore_1" << std::endl; break;
            case 0x4d: std::cout << "astore_2" << std::endl; break;
            case 0x4e: std::cout << "astore_3" << std::endl; break; 

            case 0x4f: std::cout << "iastore" << std::endl; break; // Arrays
            case 0x2e: std::cout << "iaload" << std::endl; break; // Arrays


            // --- Opcodes com 1 argumento (u1) ---
            case 0x10: { /* bipush */ 
                int8_t byte_val = (int8_t)get_u1(); 
                std::cout << "bipush " << (int)byte_val << std::endl; 
                break; 
            }
            case 0x12: { /* ldc */ 
                uint8_t index = get_u1(); 
                std::cout << "ldc #" << (int)index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl; 
                break; 
            }
            case 0x15: { /* iload */ 
                uint8_t index = get_u1(); 
                std::cout << "iload " << (int)index << std::endl; 
                break; 
            }
            case 0x36: { /* istore */ 
                uint8_t index = get_u1(); 
                std::cout << "istore " << (int)index << std::endl; 
                break; 
            }
            case 0x37: { /* lstore */ 
                uint8_t index = get_u1(); 
                std::cout << "lstore " << (int)index << std::endl; 
                break; 
            }
            case 0x3a: { /* astore */ 
                uint8_t index = get_u1(); 
                std::cout << "astore " << (int)index << "\t\t// " << resolver_indice_cp_completo(pool, index) << std::endl;
                break; 
            }
            
            // --- Opcodes com 2 argumentos (u2 - índice do CP ou s2 offset) ---
            case 0x11: { /* sipush */ 
                int16_t short_val = get_s2(); 
                std::cout << "sipush " << short_val << std::endl; 
                break;
            }
            case 0x14: { /* ldc2_w */ 
                uint16_t index = get_u2(); 
                std::cout << "ldc2_w #" << index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl; 
                break; 
            }
            case 0xb2: // getstatic
            {
                uint16_t index = get_u2();
                std::cout << "getstatic #" << index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl;
                break;
            }
            case 0xb6: // invokevirtual
            {
                uint16_t index = get_u2();
                std::cout << "invokevirtual #" << index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl;
                break;
            }
            case 0xb7: // invokespecial
            {
                uint16_t index = get_u2();
                std::cout << "invokespecial #" << index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl;
                break;
            }
            case 0xb8: // invokestatic
            {
                uint16_t index = get_u2();
                std::cout << "invokestatic #" << index << " \t// " << resolver_indice_cp_completo(pool, index) << std::endl;
                break;
            }
            case 0xa7: // goto
            {
                int16_t offset_s16 = get_s2();
                std::cout << "goto " << (offset + offset_s16) << std::endl;
                break;
            }
            case 0x9f: case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: // if_icmpeq a if_icmple
            {
                int16_t offset_s16 = get_s2();
                std::string mnemonic;
                if (opcode == 0x9f) mnemonic = "if_icmpeq"; else if (opcode == 0xa0) mnemonic = "if_icmpne";
                else if (opcode == 0xa1) mnemonic = "if_icmplt"; else if (opcode == 0xa2) mnemonic = "if_icmpge";
                else if (opcode == 0xa3) mnemonic = "if_icmpgt"; else mnemonic = "if_icmple";
                std::cout << mnemonic << " " << (offset + offset_s16) << std::endl;
                break;
            }

            // --- Opcodes com 1 argumento (u1) e 1 de tipo (u1) ---
            case 0xbc: { // newarray
                uint8_t atype = get_u1(); 
                std::cout << "newarray (" << (int)atype << ")" << std::endl; 
                break;
            }
            
            // --- Opcodes complexos ---
            case 0xc8: { /* goto_w */ 
                int32_t offset_s32 = get_s4(); 
                std::cout << "goto_w " << (offset + offset_s32) << std::endl; 
                break; 
            }
            
            default:
                std::cout << "Opcode desconhecido: 0x" << std::hex << (int)opcode << std::dec << std::endl;
                break;
        }
    }
}