#ifndef CUSTOMEFFECT_H
#define CUSTOMEFFECT_H

#include "Effect.h"
#include "Palette.h"

#include <ArduinoJson.h>
#include <FastLED.h>

class CustomEffect : public Effect {
	private:
		CRGB _color;
		
		unsigned int _numParams = 0;
		unsigned int _numTemps = 0;
		unsigned int _numVals = 0;
		unsigned int _numOps = 0;
		unsigned char *_ops;
		double *_vars = NULL;
		
		static unsigned char getReg(String num, JsonObject *operation, std::vector<double> *vals, unsigned char valsOffset)
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
		
		static unsigned char getReg(JsonObject *operation, std::vector<double> *vals, unsigned char valsOffset)
		{
			return getReg("", operation, vals, valsOffset);
		}
		
		static void buildOps(JsonArray *jsonOps, std::vector<unsigned char> *ops, std::vector<double> *vals, unsigned char valsOffset)
		{
			for (JsonVariant opVariant : *jsonOps) {
				JsonObject op = opVariant.as<JsonObject>();
				
				String name = op["name"].as<String>();
				
				if (name.equalsIgnoreCase("getColor")) {
					ops->push_back(0);
					ops->push_back(getReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("colorScaleVid")) {
					ops->push_back(1);
					ops->push_back(getReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("set")) {
					ops->push_back(2);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("add")) {
					ops->push_back(3);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("sub")) {
					ops->push_back(4);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("mul")) {
					ops->push_back(5);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
				}  else if (name.equalsIgnoreCase("div")) {
					ops->push_back(6);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("frac")) {
					ops->push_back(7);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg(&op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("pow")) {
					ops->push_back(8);
					ops->push_back(op["dest"].as<unsigned char>());
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
				} else if (name.equalsIgnoreCase("lessOrEqual")) {
					ops->push_back(9);
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
					
					unsigned int placeholderIndex = ops->size();
					ops->push_back(0); // placeholder
					
					JsonArray subOps = op["ops"].as<JsonArray>();
					buildOps(&subOps, ops, vals, valsOffset);
					
					ops->at(placeholderIndex) = ops->size() - placeholderIndex - 1;
				} else if (name.equalsIgnoreCase("greater")) {
					ops->push_back(10);
					ops->push_back(getReg("1", &op, vals, valsOffset));
					ops->push_back(getReg("2", &op, vals, valsOffset));
					
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
		static unsigned char* fromJson(String effectJson, unsigned int *length)
		{
			DynamicJsonDocument doc(4 * 1024);
			deserializeJson(doc, effectJson);
			
			unsigned int numTemps = doc["tempVars"];
			unsigned int numParams = 0;
			
			if (doc.containsKey("paramVars")) {
				JsonArray paramArr = doc["paramVars"].as<JsonArray>();
				numParams = paramArr.size();
			}
			
			std::vector<unsigned char> ops;
			std::vector<double> vals;
			
			JsonArray jsonOps = doc["ops"].as<JsonArray>();
			buildOps(&jsonOps, &ops, &vals, 2 + numParams + numTemps);
			
			unsigned int numVals = vals.size();
			unsigned int numOps = ops.size();
			
			unsigned char *buf = new unsigned char[4 * sizeof(unsigned int) + numOps + (numParams + numVals) * sizeof(double)];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], &numOps, sizeof(numOps));
			writeOff += sizeof(numOps);
			
			memcpy(&buf[writeOff], &numParams, sizeof(numParams));
			writeOff += sizeof(numParams);
			
			memcpy(&buf[writeOff], &numTemps, sizeof(numTemps));
			writeOff += sizeof(numTemps);
			
			memcpy(&buf[writeOff], &numVals, sizeof(numVals));
			writeOff += sizeof(numVals);
			
			for (int i = 0; i < numOps; i++) {
				buf[writeOff++] = ops[i];
			}
			
			if (doc.containsKey("paramVars")) {
				JsonArray paramArr = doc["paramVars"].as<JsonArray>();
				int index = 0;
				
				for (JsonVariant paramVariant : paramArr) {
					double val = paramVariant.as<double>();
					memcpy(&buf[writeOff], &val, sizeof(val));
					writeOff += sizeof(val);
				}
			}
			
			for (int i = 0; i < numVals; i++) {
				double val = vals[i];
				memcpy(&buf[writeOff], &val, sizeof(val));
				writeOff += sizeof(val);
			}
			
			*length = writeOff;
			
			return buf;
		};
		
		CustomEffect(Palette *palette, unsigned char *data, unsigned int length) : Effect("CustomEffect", palette), _color(0, 0, 0)
		{
			unsigned int readOff = 0;
			
			memcpy(&_numOps, &data[readOff], sizeof(_numOps));
			readOff += sizeof(_numOps);
			
			memcpy(&_numParams, &data[readOff], sizeof(_numParams));
			readOff += sizeof(_numParams);
			
			memcpy(&_numTemps, &data[readOff], sizeof(_numTemps));
			readOff += sizeof(_numTemps);
			
			memcpy(&_numVals, &data[readOff], sizeof(_numVals));
			readOff += sizeof(_numVals);
			
			
			Serial.println("Deserialize: " + String(_numOps) + " " + String(_numParams) + " " + String(_numTemps) + " " + String(_numVals));
			
			_ops = new unsigned char[_numOps];
			_vars = new double[2 + _numParams + _numTemps + _numVals];
			
			memcpy(_ops, &data[readOff], _numOps);
			readOff += _numOps;
			
			memcpy(_vars, &data[readOff], _numParams * sizeof(double));
			readOff += _numParams * sizeof(double);
			
			memcpy(&_vars[2 + _numParams + _numTemps], &data[readOff], _numVals * sizeof(double));
			readOff += _numVals * sizeof(double);
			
			Serial.println("Parsing Custom Effect: " + String(length) + " " + String(readOff));
		}
	
		CustomEffect(Palette *palette, String effectJson) : Effect("CustomEffect", palette), _color(0, 0, 0)
		{
			DynamicJsonDocument doc(4 * 1024);
			deserializeJson(doc, effectJson);
			
			_numTemps = doc["tempVars"];
			_numParams = 0;
			
			if (doc.containsKey("paramVars")) {
				JsonArray paramArr = doc["paramVars"].as<JsonArray>();
				_numParams = paramArr.size();
			}
			
			std::vector<unsigned char> ops;
			std::vector<double> vals;
			
			JsonArray jsonOps = doc["ops"].as<JsonArray>();
			buildOps(&jsonOps, &ops, &vals, 2 + _numParams + _numTemps);
			
			_numVals = vals.size();
			_numOps = ops.size();
			
			_ops = new unsigned char[_numOps];
			_vars = new double[2 + _numParams + _numTemps + _numVals];
			
			Serial.println("NumOps: " + String(_numOps));
			Serial.println("NumVals: " + String(_numVals));
			
			for (int i = 0; i < _numOps; i++) {
				// Serial.println("#" + String(i) + ": " + String(ops[i]));
				_ops[i] = ops[i];
			}
			
			if (doc.containsKey("paramVars")) {
				JsonArray paramArr = doc["paramVars"].as<JsonArray>();
				int index = 0;
				
				for (JsonVariant paramVariant : paramArr) {
					double val = paramVariant.as<double>();
					_vars[index++] = val;
				}
			}
			
			for (int i = 0; i < _numVals; i++) {
				_vars[2 + _numParams + _numTemps + i] = vals[i];
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
				
				_numParams = 0;
				_numTemps = 0;
				_numVals = 0;
			}
		}
		
		double getDuration()
		{
			if (_numParams >= 1 && _vars)
				return _vars[0]; // return the first register as the duration
			
			return 0.0;
		}
		
		CRGB update(double timeValue, double posValue)
		{
			_vars[_numParams] = timeValue;
			_vars[_numParams + 1] = posValue;
			
			handleOps();
			
			return _color;
		};
		
		unsigned char* serialize(unsigned int *length)
		{
			unsigned char *buf = new unsigned char[4 * sizeof(unsigned int) + _numOps + (_numParams + _numVals) * sizeof(double)];
			unsigned int writeOff = 0;
			
			memcpy(&buf[writeOff], &_numOps, sizeof(_numOps));
			writeOff += sizeof(_numOps);
			
			memcpy(&buf[writeOff], &_numParams, sizeof(_numParams));
			writeOff += sizeof(_numParams);
			
			memcpy(&buf[writeOff], &_numTemps, sizeof(_numTemps));
			writeOff += sizeof(_numTemps);
			
			memcpy(&buf[writeOff], &_numVals, sizeof(_numVals));
			writeOff += sizeof(_numVals);
			
			Serial.println("Serialize: " + String(_numOps) + " " + String(_numParams) + " " + String(_numTemps) + " " + String(_numVals));
			
			memcpy(&buf[writeOff], _ops, _numOps);
			writeOff += _numOps;
			
			memcpy(&buf[writeOff], _vars, _numParams * sizeof(double));
			writeOff += _numParams * sizeof(double);
			
			memcpy(&buf[writeOff], &_vars[2 + _numParams + _numTemps], _numVals * sizeof(double));
			writeOff += _numVals * sizeof(double);
			
			*length = writeOff;
			
			return buf;
		}
};

#endif // CUSTOMEFFECT_H