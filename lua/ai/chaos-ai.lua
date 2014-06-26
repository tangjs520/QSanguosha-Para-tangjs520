sgs.ai_skill_playerchosen.fentian = function(self, targets)
	local targetstable = {}
	for _, p in sgs.qlist(targets) do
		if self:isEnemy(p) then
			table.insert(targetstable, p)
		end
	end
	self:sort(targetstable)
	return targetstable[1]
end

local xintan_skill = {}
xintan_skill.name = "xintan"
table.insert(sgs.ai_skills, xintan_skill)
xintan_skill.getTurnUseCard = function(self)
	if (self.player:getPile("burn"):length() > math.max(self.room:getAlivePlayers():length() / 2, 2)) and not self.player:hasUsed("XintanCard") then
		return sgs.Card_Parse("@XintanCard=.")
	end
	return nil
end

sgs.ai_skill_use_func.XintanCard = function(card, use, self)
	self:sort(self.enemies, "hp")
	use.card = sgs.Card_Parse("@XintanCard=.")
	if use.to then use.to:append(self.enemies[1]) end
end
