#include <iostream>
#include <map>
#include <fstream>
#include <algorithm>
#include <vector>
#include <list>
#include <string>
#include <cstdio>
using namespace std;

class Interval
{
private:
	int l, u;
	unsigned int key;
	char c;
public:
	Interval(int lBound = 0, int uBound = 0, unsigned int k = 0, char sym = 0)
	{
		l = lBound;
		u = uBound;
		key = k;
		c = sym;
	}
	~Interval() = default;
	int getL()
	{
		return l;
	}
	int getU()
	{
		return u;
	}
	unsigned int getKey() const
	{
		return key;
	}
	char getC()
	{
		return c;
	}
	void setL(int newL)
	{
		l = newL;
	}
	void setU(int newU)
	{
		u = newU;
	}
	void setKey(unsigned int k)
	{
		key = k;
	}
	void setC(char newC)
	{
		c = newC;
	}
	friend ostream & operator<< (ostream &out, Interval &that)
	{
		out << "Interval for symbol <" << that.c << "> is [" << that.l << "; " << that.u << ") and key = " << that.key << endl;
		return out;
	}
};

struct check
{
	bool operator() (const Interval i1, const Interval i2) const
	{
		return i1.getKey() > i2.getKey();
	}
};

typedef unsigned short ushort;

string comp = "compressedText.txt",
decomp = "decompressedText.txt",
original = "originalText.txt";

const ushort qtr1 = (65535 + 1) / 4, half = qtr1 * 2, qtr3 = qtr1 * 3;

map<char, int> tabCreater(string fileName) // функци€ создани€ ассоциативного массива
{
	ifstream fin(fileName, ios::binary);
	map<char, int> tab;
	if (fin.is_open())
	{
		while (1)
		{
			unsigned char c = fin.get();
			if (fin.eof()) break;
			tab[c]++;
		}
	}
	fin.close();
	return tab;
}

void zip(list<Interval> intervals, map<char, int> tab)
{
	ofstream compressed(comp, ios::binary); // дл€ записи
	int tabSize = tab.size(); // кол-во различных символов
	
	compressed.write((char*)&tabSize, sizeof(tabSize));
	for (auto ix = tab.begin(); ix != tab.end(); ix++) // записываем данные с таблицы частотности
	{
		compressed.write((char*)&ix->first, sizeof(ix->first));
		compressed.write((char*)&ix->second, sizeof(ix->second));
	}
	
	ushort l = 0, u = 65535; // начальные присвоени€
	int divisor = intervals.back().getU(), // кол-во интервалов
		bitsToFollow = 0,   //число битов сколько нужно сбросить
		counter = 0;
	ifstream orig(original, ios::binary); // чтение оригинала
	if (!orig.is_open()) exit(0);
	char toWrite = 0;
	while (1)
	{
		int current = orig.get();
		if (current == EOF) break;
		list<Interval>::iterator iter = intervals.begin();
		for (; iter != intervals.end() && iter->getC() != current; iter++); // ищем нужный полуинтервал 
		int range = u - l + 1; // ширина нужного полуинтервала
		u = l + iter->getU() * range / divisor - 1; // пересчитываем новые границы 
		l += iter->getL() * range / divisor; 
		while (1) 
		{
			if (u < half) 
			{
				counter++;
				if (counter == 8)
				{
					compressed.put(toWrite); // записываем символ
					toWrite ^= toWrite; //обнул€ем
					counter ^= counter;
				}
				for (; bitsToFollow > 0; bitsToFollow--) //переносим найденные биты
				{
					toWrite |= (1 << (7 - counter));
					counter++;
					if (counter == 8)
					{
						compressed.put(toWrite); // записываем символ
						toWrite ^= toWrite;
						counter ^= counter;
					}
				}
			}
			else if (l >= half) // лева€ не меньше половины
			{
				toWrite |= (1 << (7 - counter));
				counter++;
				if (counter == 8)
				{
					compressed.put(toWrite);
					toWrite ^= toWrite;
					counter ^= counter;
				}
				for (; bitsToFollow > 0; bitsToFollow--)
				{
					counter++;
					if (counter == 8)
					{
						compressed.put(toWrite);
						toWrite ^= toWrite;
						counter ^= counter;
					}
				}
				u -= half;
				l -= half;
			}
			else if (l >= qtr1 && u < qtr3)
			{
				bitsToFollow++;
				u -= qtr1;
				l -= qtr1;
			}
			else break;
			u <<= 1;
			l <<= 1;
			u++;
		}
	}
	orig.close();
	compressed.close();
}

void unzip(list<Interval> intervalsToUnzip, int fullLen, ifstream &compressedF)
{
	ofstream decompressed(decomp, ios::binary);
	int divisor = intervalsToUnzip.back().getU(), // длина всего полуинтервала
		val = (compressedF.get() << 8) | compressedF.get(), // 16 бит
		counter = 0;
	char toDec = compressedF.get(); // символ, который будем расшифровывать
	ushort l = 0, u = 65535;
	while (fullLen)
	{
		list<Interval>::iterator ix = intervalsToUnzip.begin();

		unsigned int freq = (((val - l + 1) * divisor) - 1) / (u - l + 1); // считаем частоту по обратной формуле
		for (; (unsigned int) ix->getU() <= freq; ix++); // поиск слова
		int range = u - l + 1;
		u = l + ix->getU() * range / divisor - 1; // пересчитываем новые границы
		l += ix->getL() * range / divisor;
		while (1)
		{
			if (u < half)
			{
			}
			else if (l >= half)
			{
				val -= half;
				l -= half;
				u -= half;
			}
			else if (l >= qtr1 && u < qtr3)
			{
				val -= qtr1;
				l -= qtr1;
				u -= qtr1;
			}
			else break; // преобразуем кодировку в символ
			l <<= 1;
			u <<= 1;
			u++;
			val <<= 1;
			val += (toDec & (1 << (7 - counter))) >> (7 - counter);
			counter++;
			if (counter == 8)
			{
				counter ^= counter;
				toDec = compressedF.get();
			}
		}
		decompressed << ix->getC();
		fullLen--;
	}
	compressedF.close();
	decompressed.close();
}

long int fileSize(string fileName)
{
	ifstream file(fileName, ios_base::binary);  // читаем файл в двоичном виде
	file.seekg(0, ios_base::end);
	int size = (int)file.tellg();
	file.close();
	return size;
}

bool testYourLuck(string originalFile, string otherFile)
{
	if (fileSize(originalFile) != fileSize(otherFile)) return 0;

	ifstream file1(originalFile, ios::in | ios::binary);
	if (!file1.is_open()) return 0;

	ifstream file2(otherFile, ios::in | ios::binary);
	if (!file2.is_open()) return 0;

	char ch1, ch2;
	while (!file1.eof())
	{
		file1 >> ch1;
		file2 >> ch2;
		if (ch1 != ch2) 
			return 0;
	}
	return 1;
}

int main()
{
	map<char, int> tab = tabCreater(original); // таблица частот по алфавиту 
	list<Interval> intervals;
	for (map<char, int>::iterator iter = tab.begin(); iter != tab.end(); iter++)
	{
		Interval tmpI(0, 0, iter->second, iter->first); // список интервалов, без границ 
		intervals.push_back(tmpI);
	}
	intervals.sort(check());  // сортируем по кол-ву символов 
	int tmpBound = 0; // временное хранение границы
	for (auto ix = intervals.begin(); ix != intervals.end(); ix++) // устанавливаем границы интервалов
	{
		ix->setL(tmpBound);
		tmpBound += ix->getKey(); // верхн€€ граница = нижн€€ + длина промежутка
		ix->setU(tmpBound);
	}
	zip(intervals, tab);

	cout << "file " << original << " has been compressed and written in " << comp << " file" << endl;

	int fullLen = 0; // длина сжатого
	ifstream compressedFile(comp, ios::binary); // читаем
	map<char, int> freqs;
	int tabSize, key;
	char c;
	compressedFile.read((char*)&tabSize, sizeof(tabSize)); // кол-во различных символов берем из начала файла
	while (tabSize) // берем таблица частот
	{
		compressedFile.read((char*)&c, sizeof(c));
		compressedFile.read((char*)&key, sizeof(key));
		fullLen += key; // сумма длинн отрезков
		freqs[c] = key;
		tabSize--;
	}

	list<Interval> intervalsToUnzip;
	for (map<char, int>::iterator iter = freqs.begin(); iter != freqs.end(); iter++) // заполн€ем список интервалов
	{
		Interval tmpI(0, 0, iter->second, iter->first);
		intervalsToUnzip.push_back(tmpI);
	}
	intervalsToUnzip.sort(check()); // сортируем по кол-ву символов 
	tmpBound ^= tmpBound;
	for (auto ix = intervalsToUnzip.begin(); ix != intervalsToUnzip.end(); ix++)
	{
		ix->setL(tmpBound);
		tmpBound += ix->getKey(); // верхн€€ граница = нижн€€ + длина промежутка
		ix->setU(tmpBound);
	}
	unzip(intervalsToUnzip, fullLen, compressedFile);

	cout << "file '" << comp << "' has been decompressed and written in '" << decomp << "' file" << endl << endl;

	if (testYourLuck(original, decomp)) cout << "JOB HAS DONE" << endl;
	else cout << "Comparing of texts failed" << endl;

	return 0;
}
