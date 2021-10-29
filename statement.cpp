#include "statement.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {

    const auto it = closure.insert_or_assign(var_name_,expression_->Execute(closure,context));
    return it.first->second;
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    :var_name_(std::move(var))
    ,expression_(std::move(rv)){

}

VariableValue::VariableValue(const std::string& var_name) {
    dotted_ids_.push_back(var_name);
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    :dotted_ids_(std::move(dotted_ids)) {
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    if(dotted_ids_.size() == 1){
        const auto it = closure.find(dotted_ids_[0]);
        if(it != closure.end()){
            return  it->second;
        }else{
            throw std::runtime_error("ERROR: Unknown name"s);
        }
    }

    ObjectHolder obj = closure.at(dotted_ids_[0]);
  auto* class_ptr = obj.TryAs<runtime::ClassInstance>();
    if(!class_ptr){
        throw std::runtime_error("ERROR:Accessing a non-existent field"s);
    }
    for (size_t i = 1; i + 1 < dotted_ids_.size(); ++i){

        obj = obj.TryAs<runtime::ClassInstance>()->Fields().at(dotted_ids_[i]);

    }
    const auto& fields = obj.TryAs<runtime::ClassInstance>()->Fields();
    if (const auto it =fields.find(dotted_ids_.back());it!=fields.end()){
        return it->second;
    }
    throw std::runtime_error("ERROR: Unknown name"s);
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return std::make_unique<Print>(std::make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument){
    args_.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    :args_(std::move(args)) {
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    bool first = true;
    for(const auto& arg : args_ ){
        const ObjectHolder value = arg->Execute(closure,context);
        if(!first){
            context.GetOutputStream() << ' ';
        }
        if(value){
            value->Print(context.GetOutputStream(),context);
        }else{
            context.GetOutputStream() << "None";
        }
        if(args_.size() == 1)
            break;
        first = false;
    }
    context.GetOutputStream() << "\n";
    return ObjectHolder::None();
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
    :object_(std::move(object))
    ,method_(std::move(method))
    ,args_(std::move(args)) {
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    std::vector<runtime::ObjectHolder> object_args;
    for (const auto &arg : args_){
        object_args.push_back(arg->Execute(closure, context));
    }

     auto* cls = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
    if(!cls)

        throw std::runtime_error("ERROR:accessing a non-existent field");
    return cls->Call(method_, object_args, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    const auto arg =GetArg()->Execute(closure,context);
    std::ostringstream out;
    if(const auto ptr = arg.TryAs<runtime::Number>()){
        ptr->Print(out,context);
        return ObjectHolder::Own(runtime::String(out.str()));
    }else if(const auto ptr = arg.TryAs<runtime::String>()){
              ptr->Print(out,context);
        return ObjectHolder::Own(runtime::String(out.str()));
    }else if(const auto ptr = arg.TryAs<runtime::Bool>()){
        ptr->Print(out,context);
        return ObjectHolder::Own(runtime::String(out.str()));
    }else if(const auto ptr = arg.TryAs<runtime::ClassInstance>()){

        ptr->Print(out,context);
        return ObjectHolder::Own(runtime::String(out.str()));
    }else{
        return ObjectHolder::Own(runtime::String("None"));
    }
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    using namespace runtime;
    const auto lhs_arg = GetLhs()->Execute(closure,context);
    const auto rhs_arg = GetRhs()->Execute(closure,context);
    if(lhs_arg.TryAs<Number>() && rhs_arg.TryAs<Number>()){
        return ObjectHolder::Own(Number(lhs_arg.TryAs<Number>()->GetValue() +rhs_arg.TryAs<Number>()->GetValue() ));
    }else if(lhs_arg.TryAs<String>() && rhs_arg.TryAs<String>()){
        return ObjectHolder::Own(String(lhs_arg.TryAs<String>()->GetValue() +rhs_arg.TryAs<String>()->GetValue() ));
    }else if(lhs_arg.TryAs<ClassInstance>()){
        if(lhs_arg.TryAs<ClassInstance>()->HasMethod(ADD_METHOD,1)){
            return lhs_arg.TryAs<ClassInstance>()->Call(ADD_METHOD,{rhs_arg},context);
        }
    }
    throw std::runtime_error("ERROR:Incorrect operation"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    using namespace runtime;
    const auto lhs_arg_ptr = GetLhs()->Execute(closure,context).TryAs<Number>();
    const auto rhs_arg_ptr = GetRhs()->Execute(closure,context).TryAs<Number>();

    if(lhs_arg_ptr && rhs_arg_ptr){
        return ObjectHolder::Own(Number{lhs_arg_ptr->GetValue() - rhs_arg_ptr->GetValue()});
    }

    throw  std::runtime_error("ERROR: Incorrect operation"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    using namespace runtime;
    const auto lhs_arg_ptr = GetLhs()->Execute(closure,context).TryAs<Number>();
    const auto rhs_arg_ptr = GetRhs()->Execute(closure,context).TryAs<Number>();

    if(lhs_arg_ptr && rhs_arg_ptr){
        return ObjectHolder::Own(Number{lhs_arg_ptr->GetValue() * rhs_arg_ptr->GetValue()});
    }

    throw  std::runtime_error("ERROR: Incorrect operation"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    using namespace runtime;
    const auto lhs_arg_ptr = GetLhs()->Execute(closure,context).TryAs<Number>();
    const auto rhs_arg_ptr = GetRhs()->Execute(closure,context).TryAs<Number>();

    if(lhs_arg_ptr && rhs_arg_ptr){
        if(rhs_arg_ptr->GetValue() == 0){
            throw  std::runtime_error("ERROR: division by 0"s);
        }
        return ObjectHolder::Own(Number{lhs_arg_ptr->GetValue() / rhs_arg_ptr->GetValue()});
    }

    throw  std::runtime_error("ERROR: Incorrect operation"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for(const auto& arg: instructions_){
        arg->Execute(closure,context);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure,context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    : cls_(std::move(cls)){
    }

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
    :object_(std::move(object))
    ,field_name_(std::move(field_name))
    ,expression_(std::move(rv)){
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {

    auto* cls = object_.Execute(closure,context).TryAs<runtime::ClassInstance>();

    if(!cls){

        throw std::runtime_error("ERROR:attempt to access a non-instance class field");
    }
    auto& fields = cls->Fields();

    fields[field_name_]= expression_->Execute(closure,context);
    return fields[field_name_];
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
                :condition_(std::move(condition))
                ,if_body_(std::move(if_body))
                ,else_body_(std::move(else_body)) {
    }

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    auto inst_Ptr = condition_->Execute(closure,context).TryAs<runtime::Bool>();
    if(!inst_Ptr)

        throw std::runtime_error("ERROR: value does not bool value");

    if(inst_Ptr->GetValue()){
        return if_body_->Execute(closure,context);
    }else if(else_body_){
        return else_body_->Execute(closure,context);
    }
    return ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_arg = GetLhs()->Execute(closure, context);
    auto inst_Ptr = lhs_arg.TryAs<runtime::Bool>();
    if(!inst_Ptr)

        throw std::runtime_error("ERROR: value does not bool value");

    if (inst_Ptr->GetValue())
    {
        return lhs_arg;
    }
    return GetRhs()->Execute(closure, context);

}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_arg = GetLhs()->Execute(closure, context);
    auto inst_Ptr = lhs_arg.TryAs<runtime::Bool>();
    if(!inst_Ptr)

        throw std::runtime_error("ERROR: value does not bool value");

    if (!inst_Ptr->GetValue()){
        return lhs_arg;
    }
    return GetRhs()->Execute(closure, context);
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    auto inst_Ptr = GetArg()->Execute(closure,context).TryAs<runtime::Bool>();
    if(!inst_Ptr)

        throw std::runtime_error("ERROR: value does not bool value");

    return  ObjectHolder::Own(runtime::Bool{!inst_Ptr->GetValue()});
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs))
    ,cmp_(std::move(cmp)) {
    }

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    return ObjectHolder::Own(runtime::Bool(cmp_(GetLhs()->Execute(closure,context),GetRhs()->Execute(closure,context),context)));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    :_class_(class_)
    ,args_(std::move(args)){
}

NewInstance::NewInstance(const runtime::Class& class_)
    :_class_(class_) {
    }

ObjectHolder NewInstance::Execute(Closure& closure, Context& context){
    ObjectHolder oh = ObjectHolder::Own(runtime::ClassInstance(_class_));
    auto class_inst_ = oh.TryAs<runtime::ClassInstance>();
    if(class_inst_->HasMethod(INIT_METHOD,args_.size())){
        std::vector<runtime::ObjectHolder> new_args;
        for(const auto& arg: args_){
            new_args.push_back(arg->Execute(closure,context));
        }
     class_inst_->Call(INIT_METHOD,new_args,context);
    }
    return oh;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
    :body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try{
        return body_->Execute(closure,context);
    }catch (ObjectHolder& obj) {
        return std::move(obj);
    }
    return ObjectHolder::None();
}

}  // namespace ast
