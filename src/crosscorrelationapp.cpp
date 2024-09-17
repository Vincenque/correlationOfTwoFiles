#include <Python.h>
#include "crosscorrelationapp.h"
#include "matplotlibcpp.h"
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace plt = matplotlibcpp;
std::mutex progress_mutex;

void read_iq_samples(const std::string &filename, std::vector<std::complex<float>> &iq_samples, size_t start_sample, size_t num_samples)
{
	std::cout << "Attempting to open file: " << filename << "\n";
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Failed to open file: " << filename << std::endl;
		return;
	}

	// Determine file size in bytes
	std::streamsize file_size = file.tellg();

	// Each sample consists of two shorts (I and Q), so each sample is 4 bytes
	size_t total_samples_in_file = static_cast<size_t>(file_size / (sizeof(short) * 2));

	std::cout << "Total samples in file " << filename << ": " << total_samples_in_file << "\n";

	// Adjust file position for starting sample
	file.seekg(start_sample * sizeof(short) * 2, std::ios::beg); // sizeof(short) * 2 for IQ pair

	std::vector<short> short_samples(num_samples * 2);                                            // * 2 because we have I and Q pairs
	file.read(reinterpret_cast<char *>(&short_samples.front()), num_samples * sizeof(short) * 2); // Read num_samples * 2 shorts

	iq_samples.resize(num_samples);
	for (size_t i = 0; i < num_samples; ++i)
	{
		iq_samples[i] = std::complex<float>(short_samples[2 * i], short_samples[2 * i + 1]); // I -> real, Q -> imag
	}

	file.close();
	std::cout << "Completed reading samples from file: " << filename << "\n";

	// Print I and Q values of the first loaded sample
	if (!iq_samples.empty())
	{
		std::cout << "First sample - I: " << iq_samples.front().real() << ", Q: " << iq_samples.front().imag() << "\n";
		std::cout << "Last sample - I: " << iq_samples.back().real() << ", Q: " << iq_samples.back().imag() << "\n";
	}
}

void crosscorrelation_thread(const std::vector<std::complex<float>> &x, const std::vector<std::complex<float>> &y,
                             std::vector<std::complex<float>> &result, int start, int end, int total_size)
{
	for (int lag = start; lag < end; ++lag)
	{
		std::complex<float> sum(0.0, 0.0);
		if (lag >= 0)
		{
			for (int i = 0; i < total_size - lag; ++i)
			{
				sum += x[i] * std::conj(y[i + lag]);
			}
		} else
		{
			for (int i = 0; i < total_size + lag; ++i)
			{
				sum += x[i - lag] * std::conj(y[i]);
			}
		}
		result[lag + total_size - 1] = sum;

		if (lag % 100 == 0)
		{
			std::lock_guard<std::mutex> lock(progress_mutex);
			std::cout << "Progress: " << (100.0 * (lag - start) / (end - start)) << "%\r" << std::flush;
		}
	}
}

void calculate_crosscorrelation(const std::vector<std::complex<float>> &x, const std::vector<std::complex<float>> &y,
                                std::vector<std::complex<float>> &result, int num_threads)
{
	int total_size = x.size();
	int full_size = 2 * total_size - 1;
	int part_size = full_size / num_threads;

	std::vector<std::thread> threads;
	std::cout << "Starting cross-correlation calculation with " << num_threads << " threads\n";
	for (int t = 0; t < num_threads; ++t)
	{
		int start = t * part_size - total_size + 1;
		int end = (t == num_threads - 1) ? total_size - 1 : start + part_size;
		threads.emplace_back(crosscorrelation_thread, std::ref(x), std::ref(y), std::ref(result), start, end, total_size);
	}

	for (auto &thread : threads)
	{
		thread.join();
	}
	std::cout << "Cross-correlation calculation complete.\n";
}

CrossCorrelationApp::CrossCorrelationApp(QWidget *parent)
    : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	file1Edit = new QLineEdit(this);
	file1Button = new QPushButton("Select File", this);

	file2Edit = new QLineEdit(this);
	file2Button = new QPushButton("Select Second File", this);

	startSampleEdit = new QLineEdit(this);
	startSampleEdit->setText("0");
	numSamplesEdit = new QLineEdit(this);
	numSamplesEdit->setText("10000");

	calculateButton = new QPushButton("Calculate", this);

	layout->addWidget(file1Edit);
	layout->addWidget(file1Button);

	layout->addWidget(file2Edit);
	layout->addWidget(file2Button);

	layout->addWidget(new QLabel("Start Sample:", this));
	layout->addWidget(startSampleEdit);
	layout->addWidget(new QLabel("Number of Samples:", this));
	layout->addWidget(numSamplesEdit);
	layout->addWidget(calculateButton);

	connect(file1Button, &QPushButton::clicked, this, &CrossCorrelationApp::selectFile1);
	connect(file2Button, &QPushButton::clicked, this, &CrossCorrelationApp::selectFile2);
	connect(calculateButton, &QPushButton::clicked, this, &CrossCorrelationApp::calculate);

	setLayout(layout);
	setWindowTitle("Cross-Correlation Calculator");
}

CrossCorrelationApp::~CrossCorrelationApp()
{
}

void CrossCorrelationApp::selectFile1()
{
	QString defaultDir = "/home/witek/Desktop/GPS_jammer_detector/Recordings";
	QString fileName = QFileDialog::getOpenFileName(this, "Select File", defaultDir, "Binary Files (*.bin)");
	if (!fileName.isEmpty())
	{
		file1Edit->setText(fileName);
	}
}

void CrossCorrelationApp::selectFile2()
{
	QString defaultDir = "/home/witek/Desktop/GPS_jammer_detector/Recordings";
	QString fileName = QFileDialog::getOpenFileName(this, "Select Second File", defaultDir, "Binary Files (*.bin)");
	if (!fileName.isEmpty())
	{
		file2Edit->setText(fileName);
	}
}

void CrossCorrelationApp::calculate()
{
	QString file1 = file1Edit->text();
	int startSample = startSampleEdit->text().toInt();
	int numSamples = numSamplesEdit->text().toInt();

	if (file1.isEmpty())
	{
		QMessageBox::warning(this, "Input Error", "Please select the first file.");
		return;
	}

	std::vector<std::complex<float>> iq_samples1, iq_samples2, iq_samples3, iq_samples4;
	std::vector<std::complex<float>> crosscorr_result(2 * numSamples - 1);

	QString file2 = file2Edit->text();
	if (file2.isEmpty())
	{
		QMessageBox::warning(this, "Input Error", "Please select the second file.");
		return;
	}

	// 2 Files logic
	std::cout << "Reading IQ samples from file1...\n";
	read_iq_samples(file1.toStdString(), iq_samples1, startSample, numSamples);

	std::cout << "Reading IQ samples from file2...\n";
	read_iq_samples(file2.toStdString(), iq_samples2, startSample, numSamples);

	std::vector<std::complex<float>> result_ch1_ch2(2 * numSamples - 1);

	calculate_crosscorrelation(iq_samples1, iq_samples2, result_ch1_ch2, std::thread::hardware_concurrency());
	std::cout << "Cross-correlation between file1 and file2 complete.\n";

	// Find the peak correlation shift for each pair and print the phase difference in both radians and degrees
	auto find_peak_shift_and_phase = [](const std::vector<std::complex<float>> &result) {
		auto max_it = std::max_element(result.begin(), result.end(), [](const std::complex<float> &a, const std::complex<float> &b) {
			return std::abs(a) < std::abs(b);
		});
		int shift = std::distance(result.begin(), max_it) - (result.size() / 2);
		float phase_difference_radians = std::arg(*max_it);                         // Phase of the peak correlation in radians
		float phase_difference_degrees = phase_difference_radians * (180.0 / M_PI); // Convert to degrees

		std::cout << "Phase difference at maximum correlation: "
		          << phase_difference_radians << " radians, "
		          << phase_difference_degrees << " degrees\n";

		return shift;
	};

	int shift_ch1_ch2 = find_peak_shift_and_phase(result_ch1_ch2);

	// Create x_labels for plotting
	std::vector<int> x_labels(2 * numSamples - 1);
	std::iota(x_labels.begin(), x_labels.end(), -static_cast<int>(numSamples) + 1);

	// Convert complex vectors to magnitude vectors
	std::vector<float> magnitude_result_ch1_ch2(result_ch1_ch2.size());

	for (size_t i = 0; i < result_ch1_ch2.size(); ++i)
	{
		magnitude_result_ch1_ch2[i] = std::abs(result_ch1_ch2[i]);
	}

	// Convert IQ samples to interleaved format
	std::vector<float> iq_samples1_interleaved(iq_samples1.size() * 2);
	std::vector<float> iq_samples2_interleaved(iq_samples2.size() * 2);

	for (size_t i = 0; i < iq_samples1.size(); ++i)
	{
		iq_samples1_interleaved[2 * i] = iq_samples1[i].real();
		iq_samples1_interleaved[2 * i + 1] = iq_samples1[i].imag();
		iq_samples2_interleaved[2 * i] = iq_samples2[i].real();
		iq_samples2_interleaved[2 * i + 1] = iq_samples2[i].imag();
	}

	plt::figure();

	// Signal 1 plot on the left
	plt::subplot2grid(2, 2, 0, 0, 1, 1);
	plt::title("IQ interleaved samples for channel 1");
	plt::ylim(-pow(2, 11), pow(2, 11));
	plt::plot(iq_samples1_interleaved);

	// Signal 2 plot on the left
	plt::subplot2grid(2, 2, 1, 0, 1, 1);
	plt::title("IQ interleaved samples for channel 2");
	plt::ylim(-pow(2, 11), pow(2, 11));
	plt::plot(iq_samples2_interleaved);

	// Cross-correlation Ch1 vs Ch2 on the right
	plt::subplot2grid(2, 2, 0, 1, 2, 1);
	plt::title("Ch1 & Ch2 | Shift: " + std::to_string(shift_ch1_ch2));
	plt::plot(x_labels, magnitude_result_ch1_ch2);

	// Adjust layout to fit the subplots into the figure area
	plt::tight_layout();
	plt::show();
}
