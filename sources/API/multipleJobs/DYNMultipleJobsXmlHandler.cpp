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
 * @file MultipleJobsXmlHandler.cpp
 * @brief Handler for multipleJobs file : implementation file
 *
 * XmlHandler is the implementation of Dynawo handler for parsing
 * multipleJobs files.
 *
 */
#include "DYNMultipleJobsXmlHandler.h"

#include <xml/sax/parser/Attributes.h>

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator/self.hpp>
#include <boost/phoenix/bind.hpp>

#include "DYNMultipleJobs.h"
#include "DYNMultipleJobsFactory.h"
#include "DYNMarginCalculation.h"
#include "DYNScenarios.h"
#include "DYNScenario.h"

namespace lambda = boost::phoenix;
namespace lambda_args = lambda::placeholders;
namespace parser = xml::sax::parser;

xml::sax::parser::namespace_uri multipleJobs_ns("http://www.rte-france.com/dynawo");  ///< namespace used to read parameters xml file

using DYNAlgorithms::Scenarios;
using DYNAlgorithms::Scenario;
using DYNAlgorithms::MarginCalculation;
using DYNAlgorithms::LoadIncrease;

namespace multipleJobs {

XmlHandler::XmlHandler() :
multipleJobsRead_(MultipleJobsFactory::newInstance()),
scenariosHandler_(parser::ElementName(multipleJobs_ns, "scenarios")),
marginCalculationHandler_(parser::ElementName(multipleJobs_ns, "marginCalculation")) {
  onElement(multipleJobs_ns("multipleJobs/scenarios"), scenariosHandler_);
  onElement(multipleJobs_ns("multipleJobs/marginCalculation"), marginCalculationHandler_);

  scenariosHandler_.onEnd(lambda::bind(&XmlHandler::addScenarios, lambda::ref(*this)));
  marginCalculationHandler_.onEnd(lambda::bind(&XmlHandler::addMarginCalculation, lambda::ref(*this)));
}

XmlHandler::~XmlHandler() {
}

boost::shared_ptr<MultipleJobs>
XmlHandler::getMultipleJobs() {
  return multipleJobsRead_;
}

void
XmlHandler::addScenarios() {
  multipleJobsRead_->setScenarios(scenariosHandler_.get());
}

void
XmlHandler::addMarginCalculation() {
  multipleJobsRead_->setMarginCalculation(marginCalculationHandler_.get());
}

MarginCalculationHandler::MarginCalculationHandler(const elementName_type& root_element) :
scenariosHandler_(parser::ElementName(multipleJobs_ns, "scenarios")),
loadIncreaseHandler_(parser::ElementName(multipleJobs_ns, "loadIncrease")) {
  onStartElement(root_element, lambda::bind(&MarginCalculationHandler::create, lambda::ref(*this), lambda_args::arg2));

  onElement(root_element + multipleJobs_ns("scenarios"), scenariosHandler_);
  onElement(root_element + multipleJobs_ns("loadIncrease"), loadIncreaseHandler_);

  scenariosHandler_.onEnd(lambda::bind(&MarginCalculationHandler::addScenarios, lambda::ref(*this)));
  loadIncreaseHandler_.onEnd(lambda::bind(&MarginCalculationHandler::setLoadIncrease, lambda::ref(*this)));
}

MarginCalculationHandler::~MarginCalculationHandler() {
}

void
MarginCalculationHandler::create(attributes_type const& attributes) {
  marginCalculation_ = boost::shared_ptr<MarginCalculation>(new MarginCalculation());
  if (attributes["calculationType"].as_string() == "LOCAL_MARGIN")
    marginCalculation_->setCalculationType(MarginCalculation::LOCAL_MARGIN);
  else
    marginCalculation_->setCalculationType(MarginCalculation::GLOBAL_MARGIN);


  if (attributes.has("accuracy"))
    marginCalculation_->setAccuracy(attributes["accuracy"]);
}

void
MarginCalculationHandler::addScenarios() {
  marginCalculation_->setScenarios(scenariosHandler_.get());
}

void
MarginCalculationHandler::setLoadIncrease() {
  marginCalculation_->setLoadIncrease(loadIncreaseHandler_.get());
}

boost::shared_ptr<MarginCalculation>
MarginCalculationHandler::get() const {
  return marginCalculation_;
}

ScenariosHandler::ScenariosHandler(const elementName_type& root_element) :
scenarioHandler_(parser::ElementName(multipleJobs_ns, "scenario")) {
  onStartElement(root_element, lambda::bind(&ScenariosHandler::create, lambda::ref(*this), lambda_args::arg2));

  onElement(root_element + multipleJobs_ns("scenario"), scenarioHandler_);

  scenarioHandler_.onEnd(lambda::bind(&ScenariosHandler::addScenario, lambda::ref(*this)));
}

ScenariosHandler::~ScenariosHandler() {
}

void
ScenariosHandler::create(attributes_type const& attributes) {
  scenarios_ = boost::shared_ptr<Scenarios>(new Scenarios());
  scenarios_->setJobsFile(attributes["jobsFile"]);
}

void
ScenariosHandler::addScenario() {
  scenarios_->addScenario(scenarioHandler_.get());
}

boost::shared_ptr<Scenarios>
ScenariosHandler::get() const {
  return scenarios_;
}

ScenarioHandler::ScenarioHandler(const elementName_type& root_element) {
  onStartElement(root_element, lambda::bind(&ScenarioHandler::create, lambda::ref(*this), lambda_args::arg2));
}

ScenarioHandler::~ScenarioHandler() {
}

void
ScenarioHandler::create(attributes_type const& attributes) {
  scenario_ = boost::shared_ptr<Scenario>(new Scenario());
  scenario_->setId(attributes["id"]);
  if (attributes.has("dydFile"))
    scenario_->setDydFile(attributes["dydFile"]);
  if (attributes.has("criteriaFile"))
    scenario_->setCriteriaFile(attributes["criteriaFile"]);
}

boost::shared_ptr<Scenario>
ScenarioHandler::get() const {
  return scenario_;
}

LoadIncreaseHandler::LoadIncreaseHandler(const elementName_type& root_element) {
  onStartElement(root_element, lambda::bind(&LoadIncreaseHandler::create, lambda::ref(*this), lambda_args::arg2));
}

LoadIncreaseHandler::~LoadIncreaseHandler() {
}

void
LoadIncreaseHandler::create(attributes_type const& attributes) {
  loadIncrease_ = boost::shared_ptr<LoadIncrease>(new LoadIncrease());
  loadIncrease_->setId(attributes["id"]);
  loadIncrease_->setJobsFile(attributes["jobsFile"]);
}

boost::shared_ptr<LoadIncrease>
LoadIncreaseHandler::get() const {
  return loadIncrease_;
}

}  // namespace multipleJobs
