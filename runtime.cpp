#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;
namespace  {
const std::string STR_METHOD = "__str__"s;
const std::string EQUAL_METHOD  = "__eq__"s;
const std::string LESS_METHOD  = "__lt__"s;

}
namespace runtime
{

template<typename Type>
bool equal(const ObjectHolder &lhs, const ObjectHolder &rhs){
    return   lhs.TryAs<Type>()->GetValue() == rhs.TryAs<Type>()->GetValue();;
}
template<typename Type>
bool less(const ObjectHolder &lhs, const ObjectHolder &rhs){
    return   lhs.TryAs<Type>()->GetValue() < rhs.TryAs<Type>()->GetValue();;
}

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data))
    {
    }

    void ObjectHolder::AssertIsValid() const{
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object &object){
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto * /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None(){
        return ObjectHolder();
    }

    Object &ObjectHolder::operator*() const{
        AssertIsValid();
        return *Get();
    }

    Object *ObjectHolder::operator->() const{
        AssertIsValid();
        return Get();
    }

    Object *ObjectHolder::Get() const{
        return data_.get();
    }

    ObjectHolder::operator bool() const{
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder &object){

        if (const auto* ptr = object.TryAs<Number>()){
            return ptr->GetValue() != 0;
        }
        else if (const auto* ptr = object.TryAs<String>()){
            return !ptr->GetValue().empty();
        }
        else if (const auto* ptr = object.TryAs<Bool>()){
            return ptr->GetValue() == true;
        }else{
            return false;
        }
    }

    void ClassInstance::Print(std::ostream &os, Context &context){
        if (HasMethod(STR_METHOD, 0))
            Call(STR_METHOD, {}, context).Get()->Print(os, context);
        else
            os << this;
    }

    bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const{
     const runtime::Method* const method_ptr = class_.GetMethod(method);
        return method_ptr != nullptr && method_ptr->formal_params.size() == argument_count;
    }

    Closure &ClassInstance::Fields(){
        return fields_;
    }

    const Closure &ClassInstance::Fields() const{
        return fields_;
    }

    ClassInstance::ClassInstance(const Class &cls)
        : class_(cls){
    }

    ObjectHolder ClassInstance::Call(const std::string &method,
                                     const std::vector<ObjectHolder> &actual_args,
                                     Context &context){
        if (!HasMethod(method, actual_args.size())){
            throw std::runtime_error("ERROR:Такого метода не существует"s);
        }
        runtime::Closure args;
        args["self"s] = ObjectHolder::Share(*this);
        const runtime::Method* const method_ptr = class_.GetMethod(method);
        for (size_t i = 0; i < actual_args.size(); ++i){
            args[method_ptr->formal_params[i]] = actual_args[i];
        }
        return method_ptr->body->Execute(args, context);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class *parent)
        : name_(std::move(name))
        , methods_(std::move(methods))
        , parent_(parent)
    {
    }

    const Method *Class::GetMethod(const std::string &name) const{

        auto it = std::find_if(methods_.begin(), methods_.end(), [&name](const auto &method){ return method.name == name; });


        if (it != methods_.end()){
            return &*it;
        }else if(parent_ != nullptr){

            auto method_ptr = parent_->GetMethod(name);
            if (method_ptr != nullptr)
                return method_ptr;
        }
        return nullptr;
    }

    [[nodiscard]] const std::string &Class::GetName() const{
        return name_;
    }

    void Class::Print(ostream &os, [[maybe_unused]] Context &context){
        os << "Class "s << GetName();
    }

    void Bool::Print(std::ostream &os, [[maybe_unused]] Context &context){
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()){
            return equal<Number>(lhs,rhs);
            //return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
        }
        else if (lhs.TryAs<String>() && rhs.TryAs<String>()){
            return equal<String>(lhs,rhs);
            //return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
        }
        else if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
             return equal<Bool>(lhs,rhs);
            // return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
        }
        else if (!lhs && !rhs){
            return true;
        }
        else if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod(EQUAL_METHOD, 1)){
            return lhs.TryAs<ClassInstance>()->Call(EQUAL_METHOD, {rhs}, context).TryAs<Bool>()->GetValue();
        }else{
        throw std::runtime_error("ERROR:These objects cannot be compared"s);
        }
    }

    bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()){
            return less<Number>(lhs,rhs);
            //return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
        }
        else if (lhs.TryAs<String>() && rhs.TryAs<String>()){
            return less<String>(lhs,rhs);
            //return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
        }
        else if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
            return less<Bool>(lhs,rhs);
            //return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
        }
        else if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod(LESS_METHOD, 1)){
            return lhs.TryAs<ClassInstance>()->Call(LESS_METHOD, {rhs}, context).TryAs<Bool>()->GetValue();
        }
        throw std::runtime_error("ERROR:These objects cannot be compared by less"s);
    }

    bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Greater(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Less(lhs, rhs, context);
    }

} // namespace runtime
