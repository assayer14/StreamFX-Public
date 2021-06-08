// Modern effects for a modern Streamer
// Copyright (C) 2019 Michael Fabian Dirks
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#include "gfx-blur-dual-filtering.hpp"
#include <stdexcept>
#include "obs/gs/gs-helper.hpp"
#include "plugin.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <obs.h>
#include <obs-module.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Dual Filtering Blur
//
// This type of Blur uses downsampling and upsampling and clever math. That makes it less
//  controllable compared to other blur, but it can still be worked with. The distance for
//  sampling texels has to be adjusted to match the correct value so that lower levels of
//  blur than 2^n are possible.
//
// That means that for a blur size of:
//   0: No Iterations, straight copy.
//   1: 1 Iteration (2x), Arm Size 2, Offset Scale 1.0
//   2: 2 Iteration (4x), Arm Size 3, Offset Scale 0.5
//   3: 2 Iteration (4x), Arm Size 4, Offset Scale 1.0
//   4: 3 Iteration (8x), Arm Size 5, Offset Scale 0.25
//   5: 3 Iteration (8x), Arm Size 6, Offset Scale 0.5
//   6: 3 Iteration (8x), Arm Size 7, Offset Scale 0.75
//   7: 3 Iteration (8x), Arm Size 8, Offset Scale 1.0
//   ...

#define ST_MAX_LEVELS 16

streamfx::gfx::blur::dual_filtering_data::dual_filtering_data()
{
	auto gctx = streamfx::obs::gs::context();
	try {
		_effect = streamfx::obs::gs::effect::create(
			streamfx::data_file_path("effects/blur/dual-filtering.effect").u8string());
	} catch (...) {
		DLOG_ERROR("<gfx::blur::box_linear> Failed to load _effect.");
	}
}

streamfx::gfx::blur::dual_filtering_data::~dual_filtering_data()
{
	auto gctx = streamfx::obs::gs::context();
	_effect.reset();
}

streamfx::obs::gs::effect streamfx::gfx::blur::dual_filtering_data::get_effect()
{
	return _effect;
}

streamfx::gfx::blur::dual_filtering_factory::dual_filtering_factory() {}

streamfx::gfx::blur::dual_filtering_factory::~dual_filtering_factory() {}

bool streamfx::gfx::blur::dual_filtering_factory::is_type_supported(::streamfx::gfx::blur::type type)
{
	switch (type) {
	case ::streamfx::gfx::blur::type::Area:
		return true;
	default:
		return false;
	}
}

std::shared_ptr<::streamfx::gfx::blur::base>
	streamfx::gfx::blur::dual_filtering_factory::create(::streamfx::gfx::blur::type type)
{
	switch (type) {
	case ::streamfx::gfx::blur::type::Area:
		return std::make_shared<::streamfx::gfx::blur::dual_filtering>();
	default:
		throw std::runtime_error("Invalid type.");
	}
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_min_size(::streamfx::gfx::blur::type)
{
	return double_t(1.);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_step_size(::streamfx::gfx::blur::type)
{
	return double_t(1.);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_max_size(::streamfx::gfx::blur::type)
{
	return double_t(ST_MAX_LEVELS);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_min_angle(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_step_angle(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_max_angle(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

bool streamfx::gfx::blur::dual_filtering_factory::is_step_scale_supported(::streamfx::gfx::blur::type)
{
	return false;
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_min_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_step_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_max_step_scale_x(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_min_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_step_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

double_t streamfx::gfx::blur::dual_filtering_factory::get_max_step_scale_y(::streamfx::gfx::blur::type)
{
	return double_t(0);
}

std::shared_ptr<::streamfx::gfx::blur::dual_filtering_data> streamfx::gfx::blur::dual_filtering_factory::data()
{
	std::unique_lock<std::mutex>                                ulock(_data_lock);
	std::shared_ptr<::streamfx::gfx::blur::dual_filtering_data> data = _data.lock();
	if (!data) {
		data  = std::make_shared<::streamfx::gfx::blur::dual_filtering_data>();
		_data = data;
	}
	return data;
}

::streamfx::gfx::blur::dual_filtering_factory& streamfx::gfx::blur::dual_filtering_factory::get()
{
	static ::streamfx::gfx::blur::dual_filtering_factory instance;
	return instance;
}

streamfx::gfx::blur::dual_filtering::dual_filtering()
	: _data(::streamfx::gfx::blur::dual_filtering_factory::get().data()), _size(0), _size_iterations(0)
{
	auto gctx = streamfx::obs::gs::context();
	_rts.resize(ST_MAX_LEVELS + 1);
	for (std::size_t n = 0; n <= ST_MAX_LEVELS; n++) {
		gs_color_format cf = GS_RGBA;
#if 0
		cf = GS_RGBA16F;
#elif 0
		cf = GS_RGBA32F;
#endif
		_rts[n] = std::make_shared<streamfx::obs::gs::rendertarget>(cf, GS_ZS_NONE);
	}
}

streamfx::gfx::blur::dual_filtering::~dual_filtering() {}

void streamfx::gfx::blur::dual_filtering::set_input(std::shared_ptr<::streamfx::obs::gs::texture> texture)
{
	_input_texture = texture;
}

::streamfx::gfx::blur::type streamfx::gfx::blur::dual_filtering::get_type()
{
	return ::streamfx::gfx::blur::type::Area;
}

double_t streamfx::gfx::blur::dual_filtering::get_size()
{
	return _size;
}

void streamfx::gfx::blur::dual_filtering::set_size(double_t width)
{
	_size            = width;
	_size_iterations = size_t(round(width));
	if (_size_iterations >= ST_MAX_LEVELS) {
		_size_iterations = ST_MAX_LEVELS;
	}
}

void streamfx::gfx::blur::dual_filtering::set_step_scale(double_t, double_t) {}

void streamfx::gfx::blur::dual_filtering::get_step_scale(double_t&, double_t&) {}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::dual_filtering::render()
{
	auto gctx = streamfx::obs::gs::context();

#ifdef ENABLE_PROFILING
	auto gdmp = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Dual-Filtering Blur");
#endif

	auto effect = _data->get_effect();
	if (!effect) {
		return _input_texture;
	}

	std::size_t actual_iterations = _size_iterations;

	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_color(true, true, true, true);
	gs_enable_blending(false);
	gs_enable_depth_test(false);
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_set_cull_mode(GS_NEITHER);
	gs_depth_function(GS_ALWAYS);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);

	uint32_t width  = _input_texture->get_width();
	uint32_t height = _input_texture->get_height();

	// Downsample
	for (std::size_t n = 1; n <= actual_iterations; n++) {
#ifdef ENABLE_PROFILING
		auto gdm = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Down %" PRIuMAX, n);
#endif

		// Select Texture
		std::shared_ptr<streamfx::obs::gs::texture> tex_cur;
		if (n > 1) {
			tex_cur = _rts[n - 1]->get_texture();
		} else { // Idx 0 is a simply considered as a straight copy of the original and not rendered to.
			tex_cur = _input_texture;
		}

		// Reduce Size
		uint32_t owidth  = width >> n;
		uint32_t oheight = height >> n;
		if ((owidth <= 0) || (oheight <= 0)) {
			actual_iterations = n - 1;
			break;
		}

		// Apply
		effect.get_parameter("pImage").set_texture(tex_cur);
		effect.get_parameter("pImageSize").set_float2(float_t(owidth), float_t(oheight));
		effect.get_parameter("pImageTexel").set_float2(1.0f / owidth, 1.0f / oheight);
		effect.get_parameter("pImageHalfTexel").set_float2(0.5f / owidth, 0.5f / oheight);

		{
			auto op = _rts[n]->render(owidth, oheight);
			gs_ortho(0., 1., 0., 1., 0., 1.);
			while (gs_effect_loop(effect.get_object(), "Down")) {
				streamfx::gs_draw_fullscreen_tri();
			}
		}
	}

	// Upsample
	for (std::size_t n = actual_iterations; n > 0; n--) {
#ifdef ENABLE_PROFILING
		auto gdm = streamfx::obs::gs::debug_marker(streamfx::obs::gs::debug_color_azure_radiance, "Up %" PRIuMAX, n);
#endif

		// Select Texture
		std::shared_ptr<streamfx::obs::gs::texture> tex_in = _rts[n]->get_texture();

		// Get Size
		uint32_t iwidth  = width >> n;
		uint32_t iheight = height >> n;
		uint32_t owidth  = width >> (n - 1);
		uint32_t oheight = height >> (n - 1);

		// Apply
		effect.get_parameter("pImage").set_texture(tex_in);
		effect.get_parameter("pImageSize").set_float2(float_t(iwidth), float_t(iheight));
		effect.get_parameter("pImageTexel").set_float2(1.0f / iwidth, 1.0f / iheight);
		effect.get_parameter("pImageHalfTexel").set_float2(0.5f / iwidth, 0.5f / iheight);

		{
			auto op = _rts[n - 1]->render(owidth, oheight);
			gs_ortho(0., 1., 0., 1., 0., 1.);
			while (gs_effect_loop(effect.get_object(), "Up")) {
				streamfx::gs_draw_fullscreen_tri();
			}
		}
	}

	gs_blend_state_pop();

	return _rts[0]->get_texture();
}

std::shared_ptr<::streamfx::obs::gs::texture> streamfx::gfx::blur::dual_filtering::get()
{
	return _rts[0]->get_texture();
}
