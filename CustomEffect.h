#ifndef CUSTOMEFFECT_H
#define CUSTOMEFFECT_H

#include "Effect.h"
#include "Palette.h"

#include <ArduinoJson.h>
#include <FastLED.h>

class CustomEffect : public Effect {
	private:
		CRGB _color;
		
		unsigned int _numOps = 0;
		unsigned char *_ops;
		double *_vars = NULL;
		
		unsigned char getUnaryReg(JsonObject *operation, std::vector<double> *vals, unsigned char valsOffset)
		{
			JsonObject op = *operation;
			unsigned char reg = 0;
			
			if (op.containsKey("reg"))
				reg = op["reg"].as<unsigned char>();
			else if (op.containsKey("val")) {
				double val = op["val"].as<double>();
				vals->push_back(val);
				reg = valsOffset + vals->size() - 1;
			}
			
			return reg;
		}
		
		unsigned char getBinaryReg(JsonObject *operation, std::vector<double> *vals, unsigned char valsOffset, unsigned char num)
		{
			JsonObject op = *operation;
			unsigned char reg = 0;
			
			if (op.containsKey("reg" + String(num)))
				reg = op["reg" + String(num)].as<unsigned char>();
			else if (op.containsKey("val" + String(num))) {
				double val = op["val" + String(num)].as<double>();
				vals->push_back(val);
				reg = valsOffset + vals->size() - 1;
			}
			
			return reg;
		}
		
		void buildOps(JsonArray *jsonOps, std::vector<unsigned char> *ops, std::vector<double> *vals, unsigned char valsOffset)
		{
			for (JsonVariant opVariant : *jsonOps) {
				JsonObject op = opVariant.as<JsonObject>();
				
				String name = op["name"].as<String>();
				
				if (name.equalsIgnoreCase("getColor")) {
					ops->push_back(0);
					ops->push_back(getUnaryReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("colorScaleVid")) {
					ops->push_back(1);
					ops->push_back(getUnaryReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("set")) {
					ops->push_back(2);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getUnaryReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("add")) {
					ops->push_back(3);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
				} else if (name.equalsIgnoreCase("sub")) {
					ops->push_back(4);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
				} else if (name.equalsIgnoreCase("mul")) {
					ops->push_back(5);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
				}  else if (name.equalsIgnoreCase("div")) {
					ops->push_back(6);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
				} else if (name.equalsIgnoreCase("frac")) {
					ops->push_back(7);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getUnaryReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("pow")) {
					ops->push_back(8);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
				} else if (name.equalsIgnoreCase("lessOrEqual")) {
					ops->push_back(9);
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
					
					unsigned int placeholderIndex = ops->size();
					ops->push_back(0); // placeholder
					
					JsonArray subOps = op["ops"].as<JsonArray>();
					buildOps(&subOps, ops, vals, valsOffset);
					
					ops->at(placeholderIndex) = ops->size() - placeholderIndex - 1;
				} else if (name.equalsIgnoreCase("greater")) {
					ops->push_back(10);
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 1));
					ops->push_back(getBinaryReg(&op, vals, valsOffset, 2));
					
					unsigned int placeholderIndex = ops->size();
					ops->push_back(0); // placeholder
					
					JsonArray subOps = op["ops"].as<JsonArray>();
					buildOps(&subOps, ops, vals, valsOffset);
					
					ops->at(placeholderIndex) = ops->size() - placeholderIndex - 1;
				}
			}
		}
		
		void handleOps()
		{
			for (int i = 0; i < _numOps; ++i) {
				// Serial.print("OP #" + String(i) + ": ");
				
				switch(_ops[i]) {
					case 0:
						{
							unsigned char valIdx = _ops[++i];
							
							// Serial.println("Get Color At " + String(_vars[valIdx]));
							_color = _palette->getColorAtPosition(_vars[valIdx]);
						}
						break;
					case 1:
						{
							unsigned char valIdx = _ops[++i];
							
							// Serial.println("Scale Color " + String(_vars[valIdx]) + " " + String((unsigned char) _vars[valIdx]));
							_color = _color.nscale8_video((unsigned char) _vars[valIdx]);
						}
						break;
					case 2:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char valIdx =_ops[++i];
					
							// Serial.println("Set [" + String(destIdx) + "] = " + String(_vars[valIdx]));
							_vars[destIdx] = _vars[valIdx];
						}
						break;
					case 3:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
						
							// Serial.println("Add [" + String(destIdx) + "] " + String(_vars[destIdx]) + " = " + String(_vars[val1Idx]) + " + " + String(_vars[val2Idx]));
							_vars[destIdx] = _vars[val1Idx] + _vars[val2Idx];
						}
						break;
					case 4:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
						
							// Serial.println("Sub [" + String(destIdx) + "] " + String(_vars[destIdx]) + " = " + String(_vars[val1Idx]) + " - " + String(_vars[val2Idx]));
							_vars[destIdx] = _vars[val1Idx] - _vars[val2Idx];
						}
						break;
					case 5:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
						
							// Serial.println("Mul [" + String(destIdx) + "] " + String(_vars[destIdx]) + " = " + String(_vars[val1Idx]) + " * " + String(_vars[val2Idx]));
							_vars[destIdx] = _vars[val1Idx] * _vars[val2Idx];
						}
						break;
					case 6:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
						
							// Serial.println("Div [" + String(destIdx) + "] " + String(_vars[destIdx]) + " = " + String(_vars[val1Idx]) + " / " + String(_vars[val2Idx]));
							_vars[destIdx] = _vars[val1Idx] / _vars[val2Idx];
						}
						break;
					case 7:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char valIdx =_ops[++i];
					
							// Serial.println("Frac [" + String(destIdx) + "] = " + String(_vars[valIdx] - (int) _vars[valIdx]) + " (" + String(_vars[valIdx]) + ")");
							_vars[destIdx] = _vars[valIdx] - (int) _vars[valIdx];
						}
						break;
					case 8:
						{
							unsigned char destIdx = _ops[++i];
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
						
							// Serial.println("Pow [" + String(destIdx) + "] = " + String(_vars[val1Idx]) + " ^ " + String(_vars[val2Idx]));
							_vars[destIdx] = pow(_vars[val1Idx], _vars[val2Idx]);
						}
						break;
					case 9:
						{
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
							unsigned char step = _ops[++i];
						
							if (_vars[val1Idx] > _vars[val2Idx]) {
								i += step;
								
								// Serial.println("Skipping");
							} else {
								// Serial.println("Not Skipping");
							}
						}
						break;
					case 10:
						{
							unsigned char val1Idx = _ops[++i];
							unsigned char val2Idx = _ops[++i];
							unsigned char step = _ops[++i];
						
							if (_vars[val1Idx] <= _vars[val2Idx]) {
								i += step;
								
								// Serial.println("Skipping");
							} else {
								// Serial.println("Not Skipping");
							}
						}
						break;
				};
			}
		}
		
	public:
		CustomEffect(Palette *palette, String effectJson) : Effect("CustomEffect", palette), _color(0, 0, 0)
		{
			DynamicJsonDocument doc(4 * 1024);
			deserializeJson(doc, effectJson);
			
			int paramVars = doc["paramVars"];
			int tempVars = doc["tempVars"];
			
			std::vector<unsigned char> ops;
			std::vector<double> vals;
			
			JsonArray jsonOps = doc["ops"].as<JsonArray>();
			buildOps(&jsonOps, &ops, &vals, 2 + paramVars + tempVars);
			
			_numOps = ops.size();
			
			_ops = new unsigned char[_numOps];
			_vars = new double[2 + paramVars + tempVars + vals.size()];
			
			Serial.println("NumOps: " + String(_numOps));
			Serial.println("NumVals: " + String(vals.size()));
			
			for (int i = 0; i < ops.size(); i++) {
				// Serial.println("#" + String(i) + ": " + String(ops[i]));
				_ops[i] = ops[i];
			}
			
			_vars[0] = 0.2;
			
			for (int i = 0; i < vals.size(); i++) {
				_vars[2 + paramVars + tempVars + i] = vals[i];
			}
			
			/*
			_vars[1] = 0.0;
			_vars[2] = 0.0;
			
			for (int i = 0; i < 2 + paramVars + tempVars + vals.size(); i++) {
				Serial.println("Val #" + String(i) + ": " + _vars[i]);
			}
			*/
		};
		
		~CustomEffect()
		{
			if (_ops) {
				delete[] _ops;
				_ops = NULL;
				
				_numOps = 0;
			}
			
			if (_vars) {
				delete[] _vars;
				_vars = NULL;
			}
		}
		
		CRGB update(double timeValue, double posValue)
		{
			_vars[1] = timeValue;
			_vars[2] = posValue;
			
			handleOps();
			
			return _color;
		};
};

#endif // CUSTOMEFFECT_H