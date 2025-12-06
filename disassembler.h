// disassembler.h

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "classfile.h" // Precisa das estruturas ConstantPool e ClassFile

// =======================================================================
// PROTÓTIPOS DA EXIBIÇÃO E DESMONTAGEM
// =======================================================================

/**
 * @brief Exibe o conteúdo completo do Constant Pool de forma legível.
 * @param pool O vetor de ConstantInfo lido do arquivo .class.
 */
void exibir_constant_pool(const ConstantPool& pool);

/**
 * @brief Exibe o cabeçalho principal e as informações da classe.
 * @param class_data A estrutura ClassFile completa.
 */
void exibir_class_info(const ClassFile& class_data);

/**
 * @brief Exibe os campos (Fields) da classe.
 * @param class_data A estrutura ClassFile completa.
 */
void exibir_fields(const ClassFile& class_data);

/**
 * @brief Itera sobre os métodos e chama a função de desmontagem para o bytecode.
 * @param class_data A estrutura ClassFile completa.
 */
void exibir_methods(const ClassFile& class_data);

/**
 * @brief Desmonta o bytecode de um método (opcode para mnemônico) e exibe.
 * @param code O vetor de bytes do atributo Code.
 * @param pool O Constant Pool para resolver referências.
 */
void desmontar_bytecode(const std::vector<uint8_t>& code, const ConstantPool& pool);

/**
 * @brief Resolve um índice do Constant Pool (CP) e retorna uma string completa e descritiva.
 * @param pool O Constant Pool.
 * @param index O índice do CP a ser resolvido.
 * @return String descritiva, incluindo nomes de classes/métodos/campos.
 */
std::string resolver_indice_cp_completo(const ConstantPool& pool, uint16_t index); // <--- PROTÓTIPO ADICIONADO PARA RESOLVER ERRO NO INTERPRETER

#endif // DISASSEMBLER_H