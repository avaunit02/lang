#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>

namespace ast {
    enum type_id : size_t {
        t_void,
        t_bool,
        u8, u16, u32, u64,
        i8, i16, i32, i64,
        f16, f32, f64,
    };
    constexpr size_t num_primitive_types = ast::type_id::f64 + 1;
    using identifier = size_t;
    struct struct_type;
    struct array_type;
    using type = std::variant<
        ast::type_id,
        std::unique_ptr<struct_type>,
        std::unique_ptr<array_type>>;
    struct field {
        ast::type_id type;
        ast::identifier identifier;
    };
    using field_list = std::vector<ast::field>;
    struct struct_type {
        ast::field_list fields;
    };
    struct array_type {
        ast::type_id element_type;
        size_t length;
    };
    static bool type_is_primitive(ast::type_id t) {
        return t < ast::num_primitive_types;
    }
    static std::string type_to_string(const ast::type& type, std::vector<std::string>& symbols_list) {
        struct type_printer_fn {
            type_printer_fn(std::vector<std::string>& sl) : symbols_list(sl) {};
            std::ostringstream s;
            std::vector<std::string>& symbols_list;
            void operator()(ast::type_id type_id) {
                if (type_is_primitive(type_id)) {
                    switch (type_id) {
                        case t_void:    { s << "void";  break; }
                        case t_bool:    { s << "bool";  break; }
                        case u8:        { s << "u8";    break; }
                        case u16:       { s << "u16";   break; }
                        case u32:       { s << "u32";   break; }
                        case u64:       { s << "u64";   break; }
                        case i8:        { s << "i8";    break; }
                        case i16:       { s << "i16";   break; }
                        case i32:       { s << "i32";   break; }
                        case i64:       { s << "i64";   break; }
                        case f16:       { s << "f16";   break; }
                        case f32:       { s << "f32";   break; }
                        case f64:       { s << "f64";   break; }
                        default:        assert(false);
                    }
                } else {
                    s << symbols_list[static_cast<size_t>(type_id) - num_primitive_types];
                }
            }
            void operator()(const std::unique_ptr<ast::struct_type>& struct_type) {
                s << "struct { ";
                for (auto field: struct_type->fields) {
                    std::invoke(*this, field.type);
                    s << " ";
                    s << symbols_list[field.identifier];
                    s << "; ";
                }
                s << "}";
            }
            void operator()(const std::unique_ptr<ast::array_type>& array_type) {
                s << "[";
                std::invoke(*this, array_type->element_type);
                s << " ";
                s << array_type->length;
                s << "]";
            }
        };
        type_printer_fn type_printer_fn{symbols_list};
        std::visit(type_printer_fn, type);
        return type_printer_fn.s.str();
    }
    static bool type_is_bool(ast::type_id t) {
        return t == t_bool;
    }
    static bool type_is_integer(ast::type_id t) {
        return t >= u8 && t <= i64;
    }
    static bool type_is_signed_integer(ast::type_id t) {
        return t >= i8 && t <= i64;
    }
    static bool type_is_unsigned_integer(ast::type_id t) {
        return t >= u8 && t <= u64;
    }
    static bool type_is_float(ast::type_id t) {
        return t >= f16 && t <= f64;
    }
    static bool type_is_number(ast::type_id t) {
        return t >= u8 && t <= f64;
    }
    static bool llvm_type_is_bool(llvm::Type* t) {
        return t->isIntegerTy(1);
    }
    static bool llvm_type_is_integer(llvm::Type* t) {
        return t->isIntegerTy() && t->getIntegerBitWidth() > 1;
    }
    static bool llvm_type_is_float(llvm::Type* t) {
        return t->isFloatingPointTy();
    }
    static bool llvm_type_is_number(llvm::Type* t) {
        return llvm_type_is_integer(t) || llvm_type_is_float(t);
    }
    static std::string llvm_type_to_string(llvm::Type* t) {
        std::string str;
        llvm::raw_string_ostream rso(str);
        t->print(rso);
        return rso.str();
    }

    static llvm::Type* type_to_llvm_type(llvm::LLVMContext &context, ast::type_id t) {
        struct type_codegen_fn {
            llvm::LLVMContext &context;
            llvm::Type* operator()(ast::type_id type_id) {
                switch (type_id) {
                    case t_void: return llvm::Type::getVoidTy(context);
                    case t_bool: return llvm::Type::getInt1Ty(context);
                    case u8:     return llvm::Type::getInt8Ty(context);
                    case u16:    return llvm::Type::getInt16Ty(context);
                    case u32:    return llvm::Type::getInt32Ty(context);
                    case u64:    return llvm::Type::getInt64Ty(context);
                    case i8:     return llvm::Type::getInt8Ty(context);
                    case i16:    return llvm::Type::getInt16Ty(context);
                    case i32:    return llvm::Type::getInt32Ty(context);
                    case i64:    return llvm::Type::getInt64Ty(context);
                    case f16:    return llvm::Type::getHalfTy(context);
                    case f32:    return llvm::Type::getFloatTy(context);
                    case f64:    return llvm::Type::getDoubleTy(context);
                    default:     assert(false);
                }
            }
            llvm::Type* operator()(const std::unique_ptr<ast::struct_type>& struct_type) {
                std::vector<llvm::Type*> fields;
                for (auto field: struct_type->fields) {
                    fields.push_back(std::invoke(*this, field.type));
                }
                bool packed = false;
                return llvm::StructType::get(context, llvm::ArrayRef<llvm::Type*>{fields}, packed);
            }
            llvm::Type* operator()(const std::unique_ptr<ast::array_type>& array_type) {
                llvm::Type* element_type = std::invoke(*this, array_type->element_type);
                return llvm::ArrayType::get(element_type, array_type->length);
            }
        };
        return std::visit(type_codegen_fn{context}, ast::type{t});
    }
}
