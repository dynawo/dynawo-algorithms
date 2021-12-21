//
// Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
// This file is part of Dynawo, an hybrid C++/Modelica open source suite
// of simulation tools for power systems.
//

/**
 * @file  DYNMultipleJobsXmlHandler.h
 *
 * @brief handler for reading multipleJobs file : header file
 *
 */
#ifndef API_MULTIPLEJOBS_DYNMULTIPLEJOBSXMLHANDLER_H_
#define API_MULTIPLEJOBS_DYNMULTIPLEJOBSXMLHANDLER_H_

#include <boost/shared_ptr.hpp>
#include <xml/sax/parser/ComposableDocumentHandler.h>
#include <xml/sax/parser/ComposableElementHandler.h>

namespace DYNAlgorithms {
class Scenario;
class Scenarios;
class MarginCalculation;
class CriticalTimeCalculation;
class LoadIncrease;
}

namespace multipleJobs {
class MultipleJobs;
/**
 * @class ScenarioHandler
 * @brief Handler used to parse scenario element
 */
class ScenarioHandler : public xml::sax::parser::ComposableElementHandler {
 public:
  /**
   * @brief Constructor
   * @param root_element complete name of the element read by the handler
   */
  explicit ScenarioHandler(elementName_type const& root_element);

  /**
   * @brief default destructor
   */
  ~ScenarioHandler();

  /**
   * @brief return the current scenario read in xml file
   * @return scenario object build thanks to infos read in xml file
   */
  boost::shared_ptr<DYNAlgorithms::Scenario> get() const;

 protected:
  /**
   * @brief called when the XML element opening tag is read
   * @param attributes attributes of the element
   */
  void create(attributes_type const& attributes);

 private:
  boost::shared_ptr<DYNAlgorithms::Scenario> scenario_;  ///< current scenario
};


/**
 * @class LoadIncreaseHandler
 * @brief Handler used to parse load increase element
 */
class LoadIncreaseHandler : public xml::sax::parser::ComposableElementHandler {
 public:
  /**
   * @brief Constructor
   * @param root_element complete name of the element read by the handler
   */
  explicit LoadIncreaseHandler(elementName_type const& root_element);

  /**
   * @brief default destructor
   */
  ~LoadIncreaseHandler();

  /**
   * @brief return the current load increase read in xml file
   * @return load increase object build thanks to infos read in xml file
   */
  boost::shared_ptr<DYNAlgorithms::LoadIncrease> get() const;

 protected:
  /**
   * @brief called when the XML element opening tag is read
   * @param attributes attributes of the element
   */
  void create(attributes_type const& attributes);

 private:
  boost::shared_ptr<DYNAlgorithms::LoadIncrease> loadIncrease_;  ///< current load increase
};

/**
 * @class ScenariosHandler
 * @brief Handler used to parse scenarios element
 */
class ScenariosHandler : public xml::sax::parser::ComposableElementHandler {
 public:
  /**
   * @brief Constructor
   * @param root_element complete name of the element read by the handler
   */
  explicit ScenariosHandler(elementName_type const& root_element);

  /**
   * @brief default destructor
   */
  ~ScenariosHandler();

  /**
   * @brief add a scenario
   */
  void addScenario();

  /**
   * @brief return the scenarios read in xml file
   * @return scenarios object build thanks to infos read in xml file
   */
  boost::shared_ptr<DYNAlgorithms::Scenarios> get() const;

 protected:
  /**
   * @brief Called when the XML element opening tag is read
   * @param attributes attributes of the element
   */
  void create(attributes_type const& attributes);

 private:
  ScenarioHandler scenarioHandler_;  ///< handler used to scenario element
  boost::shared_ptr<DYNAlgorithms::Scenarios> scenarios_;  ///< current scenarios
};

/**
 * @class MarginCalculationHandler
 * @brief Handler used to parse margin calculation element
 */
class MarginCalculationHandler : public xml::sax::parser::ComposableElementHandler {
 public:
  /**
   * @brief Constructor
   * @param root_element complete name of the element read by the handler
   */
  explicit MarginCalculationHandler(elementName_type const& root_element);

  /**
   * @brief default destructor
   */
  ~MarginCalculationHandler();

  /**
   * @brief add a scenario
   */
  void addScenarios();

  /**
   * @brief set the load increase element
   */
  void setLoadIncrease();

  /**
   * @brief return the margin calculation read in xml file
   * @return margin calculation object build thanks to infos read in xml file
   */
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> get() const;

 protected:
  /**
   * @brief Called when the XML element opening tag is read
   * @param attributes attributes of the element
   */
  void create(attributes_type const& attributes);

 private:
  ScenariosHandler scenariosHandler_;  ///< handler used to read scenarios element
  LoadIncreaseHandler loadIncreaseHandler_;  ///< handler used to read load increase element
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> marginCalculation_;  ///< current margin calculation element
};

class CriticalTimeCalculationHandler : public xml::sax::parser::ComposableElementHandler {
 public:
  /**
   * @brief Constructor
   * @param root_element complete name of the element read by the handler
   */
  explicit CriticalTimeCalculationHandler(const elementName_type& root_element);

  /**
   * @brief default destructor
   */
  ~CriticalTimeCalculationHandler();

  /**
   * @brief return the margin calculation read in xml file
   * @return margin calculation object build thanks to infos read in xml file
   */
  boost::shared_ptr<DYNAlgorithms::CriticalTimeCalculation> get() const;

 protected:
  /**
   * @brief Called when the XML element opening tag is read
   * @param attributes attributes of the element
   */
  void create(attributes_type const& attributes);

 private:
  boost::shared_ptr<DYNAlgorithms::CriticalTimeCalculation> criticalTimeCalculation_;  ///< current critical time calculation element
};

/**
 * @class XmlHandler
 * @brief Parameters file handler class
 *
 * XmlHandler is the implementation of XML handler for parsing
 * parameters files.
 */
class XmlHandler : public xml::sax::parser::ComposableDocumentHandler {
 public:
  /**
   * @brief Default constructor
   */
  XmlHandler();

  /**
   * @brief Destructor
   */
  ~XmlHandler();

  /**
   * @brief Parsed MultipleJobs instance getter
   *
   * @return MultipleJobs instance.
   */
  boost::shared_ptr<MultipleJobs> getMultipleJobs();

 private:
  /**
   * @brief add a scenarios list to the current MultipleJobs element
   */
  void addScenarios();

  /**
   * @brief add a margin calculation element to the current MultipleJobs element
   */
  void addMarginCalculation();

  /**
   * @brief add a margin calculation element to the current MultipleJobs element
   */
  void addCriticalTimeCalculation();

 private:
  boost::shared_ptr<MultipleJobs> multipleJobsRead_;  ///< MultipleJobs instance
  ScenariosHandler scenariosHandler_;  ///< handler used to read scenarios element
  CriticalTimeCalculationHandler criticalTimeCalculationHandler_;  ///< handler used to read critical time calculation element
  MarginCalculationHandler marginCalculationHandler_;  ///< handler used to read margin calculation element
};

}  // namespace multipleJobs

#endif  // API_MULTIPLEJOBS_DYNMULTIPLEJOBSXMLHANDLER_H_

