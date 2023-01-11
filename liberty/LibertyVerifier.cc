

// OpenSTA, Static Timing Analyzer
// Copyright (c) 2022, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "LibertyVerifier.hh"

#include <stdlib.h>
#include <algorithm>

#include "ConcreteNetwork.hh"
#include "FuncExpr.hh"
#include "Liberty.hh"
#include "PortDirection.hh"
#include "StaState.hh"
#include "TableModel.hh"
#include "TimingArc.hh"
#include "TimingModel.hh"
#include "TimingRole.hh"
#include "Units.hh"

namespace sta {

using std::abs;

class LibertyVerifier
{
public:
  LibertyVerifier(const LibertyLibrary *lib,
		   const char *filename,
		   Report *report);
  void verifyLiberty();
  
protected:

private:
  const LibertyLibrary *library_;
  const char *filename_;
  Report *report_;

private:
  bool writeTestVerilog();
  int print_ports(FILE *stream, const std::vector<LibertyPort *> &port_list, int port_index=0, bool is_connection = false);
};
  
void
verifyLiberty(LibertyLibrary *lib,
             const char *filename,
             StaState *sta)
{
    LibertyVerifier verifier(lib, filename, sta->report());
    verifier.verifyLiberty();
}

LibertyVerifier::LibertyVerifier(const LibertyLibrary *lib,
                                 const char *filename,
                                 Report *report):
    library_(lib),
    filename_(filename),
    report_(report)
{
}

void
LibertyVerifier::verifyLiberty()
{
  this->writeTestVerilog();
}

int LibertyVerifier::print_ports(FILE *stream, const std::vector<LibertyPort *> &port_list, int port_index/*=0*/, bool is_connection/*=false*/)
{
  int count = port_index;
  for (auto port : port_list) {
    if (count != 0)
      fprintf(stream, ", ");
    if (is_connection == true) {
      fprintf(stream, ".%s(%s)", port->name(), port->name());
    }
    else {
      fprintf(stream, "%s", port->name());
    }
    ++count;
  }
  return count;
}
/* This is what the test verilog looks like
 * module test1 (in1, in2, clk1, clk2, out1, out2);
  input in1, in2, clk1, clk2;
  output out1, out2;
  block1 b1 (.in1(in11), .in2(in21), .clk1(clk11), .clk2(clk21),
          .out1(out11), .out2(out21));
  endmodule
*/
bool LibertyVerifier::writeTestVerilog()
{
    std::string cellName = library_->name();
    std::string filename = cellName + "_test.v";
    FILE *stream= fopen(filename.c_str(), "w");
    bool status = false;
    std::vector<LibertyPort *> input_ports;
    std::vector<LibertyPort *> output_ports;

    if (stream) {
      printf("Writing test verilog");
      sta::ConcreteCell *cell = library_->findCell(cellName.c_str());
      LibertyCellPortIterator port_iter(cell->libertyCell());

      while (auto port = port_iter.next()) {
        if (port->direction()->isInput()) {
          input_ports.push_back((port));
        }
        else if (port->direction()->isOutput()) {
          output_ports.push_back((port));
        }
      }
      //================================================================
      // module statement + input/output ports
      int count = 0;
      fprintf(stream, "module %s_test (", cellName.c_str());
      count = print_ports(stream, input_ports);
      print_ports(stream, output_ports, count);
      fprintf(stream, ");\n");
      //================================================================
      // Write the port types for input and output ports
      if (input_ports.size() > 0) {
        fprintf(stream, "   input ");
        print_ports(stream, input_ports);
        fprintf(stream, ";\n");
      }
      if (output_ports.size() > 0) {
        fprintf(stream, "   output ");
        print_ports(stream, output_ports);
        fprintf(stream, ";\n");
        ++count;
      }
      //================================================================
      // Connect the cell up
      // block1 b1 (.in1(u1z), .in2(u2z), .clk1(c1z), .clk2(c2z), .out1(b1out1), .out2(b1out2));
      std::string instName = cellName + "_inst";
      fprintf(stream, "   %s %s (", cellName.c_str(), instName.c_str());
      count = print_ports(stream, input_ports, 0, true);
      print_ports(stream, output_ports, count, true);
      fprintf(stream, ");\nendmodule\n");
      fclose(stream);
      // This is where we go through the model and make up our test verilog.
      // Lets try to make it nicely formatted.
      status = true;
    }
    else {
      // throw FileNotWritable(filename);: Compile error fix later.
    }
    return status;
}

} // namespace

