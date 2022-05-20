/*
Il file wav è composto da un header di 12 byte, un chunk fmt, e uno data
Tra questi possono esserci altri chunk non essenziali
Ogni chunk inizia con 4 byte di ID e altri 4 di dimensione
*/

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int convert4B2I(char*); //converte 4 byte little endian in intero
int convert2B2I(char*); //converte 2 byte little endian in intero

int main()
{
	string nomeFileInput = "input.wav";
	char* header; //vettore di char in cui sarà messo l'header (servirà per la scrittura) 
	char* fmtChunk; //vettore per il subchunk del formato
	char* dataChunk; //per il data
	char* nextChunkID; //vettori temporanei dell ID
	char* nextChunkSize; //e del size
	char* newData; //è un data modificato, con all'inizio gli elementi di data da tenere. Il resto sono vuoti
	char* newDataHeader;
	char* fileFinale;
	int byteRate; //quanti byte per secondo  (192k, da estrarre da fmt)
	int blockAlign; //quanti byte per campione
	int finalSize = 12; //inizializzato a 12 per l'header

	//---PRIMA PARTE: ESTRARRE INFO ESSENZIALE DAL FILE---//

	ifstream fileAudioInput;
	fileAudioInput.open(nomeFileInput, ios::binary);

	if (fileAudioInput.is_open()) {

		header = new char[12];
		fileAudioInput.read(header, 12); //header RIFF salvato nel vettore header

		nextChunkID = new char[4];
		nextChunkSize = new char[4];

		fileAudioInput.read(nextChunkID, 4); //mi aspetto sia il chunck id del fmt, cioe appunto "fmt "
		fileAudioInput.read(nextChunkSize, 4); //16 byte da ora		
		string ID1(nextChunkID, 4); //trasformo in string per controllare se è fmt

		cout << nextChunkID << endl << convert4B2I(nextChunkSize) << endl;

		while (ID1.compare("fmt ") != 0)
		{
			//skippa al prossimo se non è un chunk fmt
			fileAudioInput.seekg(int(fileAudioInput.tellg()) + convert4B2I(nextChunkSize)); //sposta l'indicatore al prossimo chunk			
			fileAudioInput.read(nextChunkID, 4);
			fileAudioInput.read(nextChunkSize, 4);
			string ID(nextChunkID, 4);
			ID1 = ID;
		}
		//SCRITTURA DI FMT
		cout << (int(fileAudioInput.tellg()) - 8) << endl;
		fileAudioInput.seekg(int(fileAudioInput.tellg()) - 8); //torna indietro di 8 byte per salvare anche i primi due campi
		fmtChunk = new char[8 + convert4B2I(nextChunkSize)]; //crea il vettore di dimensione 8 + grandezza del chunk

		cout << "\n\n lunghezza di fmt: " << strlen(fmtChunk) << endl;
		fileAudioInput.read(fmtChunk, (8 + convert4B2I(nextChunkSize))); //salva gli 8 byte + i restanti del chunk
		cout << "GCOUNT:  " << fileAudioInput.istream::gcount() << endl;
		cout << fmtChunk << endl;
		finalSize += (8 + convert4B2I(nextChunkSize)); //troppe chiamate a questa funzione
		cout << "\n\n lunghezza di fmt: " << strlen(fmtChunk) << endl;

		//estrazione byteRate 
		char tempRate[4];
		for (int i = 0; i < 4; i++) {
			tempRate[i] = fmtChunk[i + 16];
		}
		byteRate = convert4B2I(tempRate);
		//estrazione blockAlign, che viene subito dopo
		char tempBlock[2];
		for (int i = 0; i < 2; i++) {
			tempBlock[i] = fmtChunk[i + 20];
		}
		blockAlign = convert2B2I(tempBlock);

		//STESSA COSA PER DATA
		fileAudioInput.read(nextChunkID, 4); //mi aspetto sia il chunck id del data, cioe appunto "data"
		fileAudioInput.read(nextChunkSize, 4);
		string ID2(nextChunkID, 4); //trasformo in string per controllare se è data

		while (ID2.compare("data") != 0)
		{
			fileAudioInput.seekg(int(fileAudioInput.tellg()) + convert4B2I(nextChunkSize)); //sposta l'indicatore al prossimo chunk
			fileAudioInput.read(nextChunkID, 4);
			fileAudioInput.read(nextChunkSize, 4);
			string ID(nextChunkID, 4);
			ID2 = ID;
		}
		//SCRITTURA DI DATA
		fileAudioInput.seekg(int(int(fileAudioInput.tellg())) - 8); //torna indietro di 8 byte per salvare anche i primi due campi
		cout << convert4B2I(nextChunkSize);
		dataChunk = new char[int(8 + convert4B2I(nextChunkSize))]; //crea il vettore di dimensione 8 + grandezza del chunk
		fileAudioInput.read(dataChunk, int(convert4B2I(nextChunkSize)) + 8); //salva gli 8 byte + i restanti del chunk

		fileAudioInput.close();

		//---SECONDA PARTE: ELIMINARE IL SILENZIO (IN DATA) ---// 
		//eliminare gli zeri (o valori bassi) quando sono troppi consecutivamente
		//controllo solo il primo dei 4 byte dei campioni
		//sono teoricamente 192kB per secondo (è indicato nel byteRate, byte 16-19 del fmt)

		int dataSize = convert4B2I(nextChunkSize);
		int inizioBlocco = 0, consecutivi = 0, newDataDim = 0;
		bool streak = false, pausa = false;
		newData = new char[dataSize];
		int sampleRate = byteRate / blockAlign;
		for (int i = 0; i < dataSize; i += blockAlign) {  // un campione per volta, solo il primo byte
			if (dataChunk[i] > 0x40 && dataChunk[i] < 0xC0) { //quando c'è audio (parametri da cambiare per il treshold), standard: dataChunk[i] > 0x10 && dataChunk[i] < 0xF0
				if (streak) {
					streak = false;
					consecutivi = 0;
				}
				if (pausa) {
					inizioBlocco = i;
					pausa = false;
				}
			}
			else { //silenzio
				if (!pausa) {
					if (!streak) {
						streak = true;
					}
					consecutivi++;
					if (consecutivi == sampleRate / 1000) { //se vi è un secondo di silenzio
						for (int j = 0; j < (i - inizioBlocco); j++) {
							newData[newDataDim + j] = dataChunk[inizioBlocco + j];
						}
						newDataDim += (i - inizioBlocco);
						pausa = true;
					}
				}
			}
		}
		if (!pausa) { //ultimo salvataggio se non è in pausa
			for (int j = 0; j < (dataSize - inizioBlocco); j++) {
				newData[newDataDim + j] = dataChunk[inizioBlocco + j];
			}
			newDataDim += (dataSize - inizioBlocco);
		}

		//---TERZA PARTE: MODIFICARE LE INFO E RICOSTRUIRE IL FILE---//
		finalSize += (8 + newDataDim);
		//cambiare chunksize: 36+newdatadim o (finalsize-8) -> esadecimale
		header[4] = ((finalSize - 8) >> 24) & 0xFF;
		header[5] = ((finalSize - 8) >> 16) & 0xFF;
		header[6] = ((finalSize - 8) >> 8) & 0xFF;
		header[7] = (finalSize - 8) & 0xFF;
		//cambiare datasize: new datadim
		dataChunk[4] = (newDataDim >> 24) & 0xFF;
		dataChunk[5] = (newDataDim >> 16) & 0xFF;
		dataChunk[6] = (newDataDim >> 8) & 0xFF;
		dataChunk[7] = newDataDim & 0xFF;
		//nuovo header per il data (lo faccio ora per poter deallocare datachunk prima di creare filefinale
		newDataHeader = new char[8];
		for (int i = 0; i < 8; i++) {
			newDataHeader[i] = dataChunk[i];
		}
		//deallocare datachunk
		delete[] dataChunk;

		//creare file finale: HEADER + FMT + PRIMI 8 BYTE DEL DATACHUNK + NEWDATA
		fileFinale = new char[finalSize];
		for (int i = 0; i < 12; i++) {
			fileFinale[i] = header[i];
		}
		for (int i = 12; i < 36; i++) {
			fileFinale[i] = fmtChunk[i - 12];
		}
		for (int i = 36; i < 44; i++) {
			fileFinale[i] = newDataHeader[i - 36];
		}
		for (int i = 44; i < finalSize; i++) {
			fileFinale[i] = newData[i - 44];
		}

		//QUARTA PARTE: STAMPA FILE//
		ofstream fileAudioOuput;
		fileAudioOuput.open("output.wav", ios::binary);

		if (fileAudioOuput.is_open()) {
			fileAudioOuput.write(fileFinale, finalSize);
			//int blocco = 1000;
			//for (int i = 0; i < finalSize; i += blocco) {
			//	fileAudioOuput.write(fileFinale, blocco);
			//}
		}

	}

	else cout << "Impossibile aprire";
	return 0;
}

int convert4B2I(char* byteinesadecimale)
{
	int size = int((unsigned char)(byteinesadecimale[3]) << 24 |
		(unsigned char)(byteinesadecimale[2]) << 16 |
		(unsigned char)(byteinesadecimale[1]) << 8 |
		(unsigned char)(byteinesadecimale[0]));
	return size;
}

int convert2B2I(char* byteinesadecimale)
{
	int size = int((unsigned char)(byteinesadecimale[1]) << 8 | (unsigned char)(byteinesadecimale[0]));
	return size;
}

