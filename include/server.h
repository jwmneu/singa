// Copyright © 2014 Wei Wang. All Rights Reserved.
// 2014-11-29 13:46

#ifndef INCLUDE_SERVER_H_
#define INCLUDE_SERVER_H_

#include "proto/model.pb.h"
namespace lapis {
/**
 * \file server.h defines the Class \class TableServer and the interface of
 * its handlers. Two classes implemented this interface are also provided,
 *  namely, \class TSHandlerForSGD and \class TSHandlerForAda.
 */

/**
 * TableServer runs a loop to handle requests from workers for a table.
 * There are three requests, Put, Get and Update. Every Table is associated
 * with a the \class Tableserverhandler (e.g., \class TSHandlerForSGD).
 */
class TableServer{
 public:
  void Start(const SGDProto & sgd);
};

/**
 * Base class, specifies the interface of request handlers of table server.
 */
class TableServerHandler: public BaseUpdateHandler<TKey, TVal>{
 public:
  virtual void Setup(const SGDProto& sgd);
  virtual bool CheckpointNow(const VKey& key, const TVal& val);

  virtual bool Update(TVal* origin, const TVal& update)=0;
  virtual bool Get(const TKey& key, const TVal &from, TVal* to)=0;
  virtual bool Put(const TKey& key, TVal* to, const TVal& from)=0;

 protected:
  int checkpoint_after_, checkpoint_frequency_;
};

/**
 * Table server handler for SGD algorithm.
 * The update considers momentum, learning rate and weight decay.
 */
class TSHandlerForSGD: public TableServerHandler {
 public:
  virtual void Setup(const SGDProto& sgd)=0;
  virtual bool Update(TVal* origin, const TVal& update);
  virtual bool Get(const TKey& key, const TVal &from, TVal* to);
  virtual bool Put(const TKey& key, TVal* to, const TVal& from);

 protected:
  float GetLearningRate(int step, float multiplier){
    float lr=UpdateHyperParam(
        step, learning_rate_change_,
        learning_rate_change_steps_, learning_rate_, gamma_);
    return lr*multiplier;
  }

  float GetWeightDecay(int step, float multiplier){
    return weight_decay_*multiplier;
  }

  float GetMomentum(int step, float multiplier){
    return momentum_;
  }

  float UpdateHyperParam(
      int step, SGDValue::ChangeProto change,
      int change_steps, float a, float b);

 protected:
   float learning_rate_, momentum_, weight_decay_, gamma_;
   int learning_rate_change_steps_;
   SGDProto_ChangeProto learning_rate_change_;
};

/**
 * Table server handler for AdaGrad SGD.
 */
class TSHandlerForAda: public TableServerHandler {
 public:
  virtual void Setup(const SGDProto& sgd)=0;
  virtual bool Update(TVal* origin, const TVal& update);
  virtual bool Get(const TKey& key, const TVal &val, TVal* ret);
  virtual bool Get(const TKey& key, const TVal &from, TVal* to);
  virtual bool Put(const TKey& key, TVal* to, const TVal& from);
};

/****************************************************************************/
/**
 * Register Tableserverhandler with type identifier
 * @param type identifier of the Tableserverhandler e.g., TSHandlerForSGD
 * @param handler the child layer class
 */
#define REGISTER_TSHandler(type, handler) TSHandlerFactory::Instance()->\
  RegisterCreateFunction(type,
                        [](void)-> TableServerHandler* {return new handler();})

/**
 * Factory for creating Tableserverhandler based on user provided type string.
 * Users are required to register user-defined Tableserverhandler before
 * creating instances of them during runtime. For example, if you define a
 * new Tableserverhandler TSHandlerForFoo with identifier "Foo", then you can
 * use it by 1) register it (e.g., at the start of the program); 2)Then call
 * TSHandlerFactory::Instance()->Create("Foo") to create an instance.
 */
class TSHandlerFactory {
 public:
  /**
   * static method to get instance of this factory
   */
  static std::shared_ptr<TSHandlerFactory> Instance();
  /**
   * Register user defined layer, i.e., add the layer type/identifier and a
   * function which creats an instance of the layer. This function is called by
   * the REGISTER_LAYER macro.
   * @param id identifier of the layer, every layer has a type to identify it
   * @param create_function a function that creates a layer instance
   */
  void RegisterCreateFunction(const std::string id,
                              std::function<Layer*(void)> create_function);
  /**
   * create a layer  instance by providing its type
   * @param type the identifier of the layer to be created
   */
  TableServerHandler *Create(const std::string id);

 private:
  //! To avoid creating multiple instances of this factory in the program
  TSHandlerFactory();
  //! Map that stores the registered Layers
  std::map<std::string, std::function<TableServerHandler*(void)>> map_;
  static std::shared_ptr<TSHandlerFactory> instance_;
};


} /* lapis */

#endif
