function normal(shader, t_base, t_second, t_detail)
	shader:begin("models_scope_glass", "models_scope_glass")
		:fog(true)
		:zb(true, false)
		:blend(true, blend.one, blend.invsrcalpha)
		:aref(true, 0)
		:sorting(2, true)

	shader:dx10texture("scope_reference_glass_spots", "$user$svp_reference_glass_spots")
	shader:dx10texture("scope_reference_glass_dirt", "$user$svp_reference_glass_surface")
	shader:dx10texture("s_base", t_base)
	shader:dx10texture("s_tonemap", "$user$tonemap")
	shader:dx10texture("s_lens_env0", "$user$env_s0")
	shader:dx10texture("s_lens_env1", "$user$env_s1")
	shader:dx10sampler("smp_base")
	shader:dx10sampler("smp_rtlinear")
end

