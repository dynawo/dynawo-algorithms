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

#include <fstream>
#include <sstream>

#include <xml/sax/formatter/AttributeList.h>

#include <DYNCommon.h>

#include "DYNAggrResXmlExporter.h"
#include "DYNMacrosMessage.h"
#include "DYNResultCommon.h"

using std::fstream;
using std::ostream;
using std::string;
using std::vector;

using xml::sax::formatter::AttributeList;
using xml::sax::formatter::Formatter;
using xml::sax::formatter::FormatterPtr;

using DYNAlgorithms::SimulationResult;
using DYNAlgorithms::LoadIncreaseResult;

namespace aggregatedResults {
void
XmlExporter::exportScenarioResultsToFile(const vector<SimulationResult>& results, const string& filePath) const {
  fstream file;
  file.open(filePath.c_str(), fstream::out);
  if (!file.is_open()) {
    throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filePath.c_str());
  }

  exportScenarioResultsToStream(results, file);
  file.close();
}

void
XmlExporter::exportScenarioResultsToStream(const vector<SimulationResult>& results, ostream& stream) const {
  FormatterPtr formatter = Formatter::createFormatter(stream, "http://www.rte-france.com/dynawo");

  formatter->startDocument();
  AttributeList attrs;

  DYNAlgorithms::status_t status = DYNAlgorithms::CONVERGENCE_STATUS;
  for (unsigned int i=0, iEnd = results.size(); i < iEnd; i++) {
    if (static_cast<int>(results[i].getStatus()) > static_cast<int>(status))
      status = results[i].getStatus();
  }
  attrs.add("status", getStatusAsString(status));
  formatter->startElement("aggregatedResults", attrs);
  appendScenarioResultsElement(results, formatter);
  formatter->endElement();  // aggregatedResults
  formatter->endDocument();
}


void
XmlExporter::exportLoadIncreaseResultsToFile(const vector<LoadIncreaseResult>& results, const string& filePath) const {
  fstream file;
  file.open(filePath.c_str(), fstream::out);
  if (!file.is_open()) {
    throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filePath.c_str());
  }

  exportLoadIncreaseResultsToStream(results, file);
  file.close();
}

void
XmlExporter::exportLoadIncreaseResultsToStream(const vector<LoadIncreaseResult>& results, std::ostream& stream) const {
  FormatterPtr formatter = Formatter::createFormatter(stream, "http://www.rte-france.com/dynawo");

  formatter->startDocument();
  AttributeList attrs;
  formatter->startElement("aggregatedResults", attrs);
  for (unsigned int i=0, iEnd = results.size(); i < iEnd; i++) {
    attrs.clear();
    const DYNAlgorithms::LoadIncreaseResult& loadIncreaseRes = results[i];
    attrs.add("loadLevel", loadIncreaseRes.getLoadLevel());
    attrs.add("status", getStatusAsString(loadIncreaseRes.getStatus()));
    formatter->startElement("loadIncreaseResults", attrs);
    if (loadIncreaseRes.getStatus() == DYNAlgorithms::CONVERGENCE_STATUS) {
      appendScenarioResultsElement(loadIncreaseRes.getResults(), formatter);
    }
    formatter->endElement();  // loadIncreaseResults
  }
  formatter->endElement();  // aggregatedResults
  formatter->endDocument();
}

void
XmlExporter::exportCriticalTimeResultsToFile(const double& criticalTime, std::string messageCriticalTimeError, const std::string& filePath) const {
  fstream file;
  file.open(filePath.c_str(), fstream::out);
  if (!file.is_open()) {
    throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filePath.c_str());
  }

  exportCriticalTimeResultsToStream(criticalTime, messageCriticalTimeError, file);
  file.close();
}

void
XmlExporter::exportCriticalTimeResultsToStream(const double& criticalTime, std::string messageCriticalTimeError, std::ostream& stream) const {
  FormatterPtr formatter = Formatter::createFormatter(stream, "http://www.rte-france.com/dynawo");

  formatter->startDocument();
  AttributeList attrs;
  formatter->startElement("aggregatedResults", attrs);
  attrs.add("criticalTime", criticalTime);
  attrs.add("message", messageCriticalTimeError);
  formatter->startElement("criticalTimeResults", attrs);
  formatter->endElement();

  formatter->endElement();  // aggregatedResults
  formatter->endDocument();
}

void
XmlExporter::appendScenarioResultsElement(const vector<SimulationResult>& results, FormatterPtr& formatter) const {
  AttributeList attrs;
  for (unsigned int i=0; i < results.size(); i++) {
    attrs.clear();
    const DYNAlgorithms::SimulationResult& result = results[i];
    attrs.add("id", result.getScenarioId());
    attrs.add("status", getStatusAsString(result.getStatus()));
    formatter->startElement("scenarioResults", attrs);
    const std::vector<std::pair<double, std::string> >& failingCriteria = result.getFailingCriteria();
    if (!failingCriteria.empty()) {
      for (std::vector<std::pair<double, std::string> >::const_iterator it = failingCriteria.begin(), itEnd = failingCriteria.end();
          it != itEnd; ++it) {
        attrs.clear();
        attrs.add("id", it->second);
        attrs.add("time", DYN::double2String(it->first));
        formatter->startElement("criterionNonRespected", attrs);
        formatter->endElement();  // criterionNonRespected
      }
    }
    formatter->endElement();  // scenarioResults
  }
}

}  // namespace aggregatedResults


